
#include "fdsetsize.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <hiredis.h>

#include "batch.h"
#include "client.h"
#include "loader.h"
#include "conf.h"

#include "statistics.h"

#define STATS_NUM_LENGTH_MAX 16
#define REDIS_KEEPALIVE_SECS 60

static void prepare_reporting_data(batch_context* bctx, unsigned long now)
{
	int i=0;
	stat_point *http_statistics;
	stat_point *https_statistics;
	stat_point *http_statistics_total;
	stat_point *https_statistics_total;
	redis_statistics_output *rs_output=&(bctx->rs_output);
	int total_client=0;

    /*total_client*/
	for (i = 0; i <= threads_subbatches_num; i++)
	{
		total_client += pending_active_and_waiting_clients_num_stat (bctx + i);
	}
	rs_output->d_total_client = total_client;

    /*cycle_interval*/
	rs_output->d_cycle_interval = (int)(now - bctx->last_measure)/1000;
	if (!rs_output->d_cycle_interval)
	{
		rs_output->d_cycle_interval = 1;
	}

	/*statistics from libcurl*/
	for (i = 0; i <= threads_subbatches_num; i++)
	{
		if (i)
		{
			stat_point_add (&bctx->http_delta, &(bctx + i)->http_delta);
			stat_point_add (&bctx->https_delta, &(bctx + i)->https_delta);

			/* Other threads statistics - reset just after collecting */
			stat_point_reset (&(bctx + i)->http_delta); 
			stat_point_reset (&(bctx + i)->https_delta);

			op_stat_point_add (&bctx->op_delta, &(bctx + i)->op_delta );
			op_stat_point_reset (&(bctx + i)->op_delta);
		}
	}

	/*total statistics*/
	stat_point_add (&bctx->http_total, &bctx->http_delta);
	stat_point_add (&bctx->https_total, &bctx->https_delta);
	op_stat_point_add (&bctx->op_total, &bctx->op_delta );

	/*caps_current*/
	rs_output->d_caps_current = bctx->op_delta.call_init_count/ rs_output->d_cycle_interval;

	http_statistics=&bctx->http_delta;
	https_statistics=&bctx->https_delta;
	http_statistics_total=&bctx->http_total;
	https_statistics_total=&bctx->https_total;

	rs_output->d_redis_keepalive_secs = REDIS_KEEPALIVE_SECS;

	rs_output->d_http_request_num=http_statistics->requests;
	rs_output->d_hf_1xx=http_statistics->resp_1xx;
	rs_output->d_hf_2xx=http_statistics->resp_2xx;
	rs_output->d_hf_3xx=http_statistics->resp_3xx;
	rs_output->d_hf_4xx=http_statistics->resp_4xx;
	rs_output->d_hf_5xx=http_statistics->resp_5xx;
	rs_output->d_hf_err=http_statistics->other_errs;
	rs_output->d_hf_t_err=http_statistics->url_timeout_errs;
	rs_output->d_hf_delay=http_statistics->appl_delay;
	rs_output->d_hf_delay_2xx=http_statistics->appl_delay_2xx;
	rs_output->d_hf_input_traffic=http_statistics->data_in/rs_output->d_cycle_interval;
	rs_output->d_hf_output_traffic=http_statistics->data_out/rs_output->d_cycle_interval;
	rs_output->d_https_request_num=https_statistics->requests;
	rs_output->d_hfs_1xx=https_statistics->resp_1xx;
	rs_output->d_hfs_2xx=https_statistics->resp_2xx;
	rs_output->d_hfs_3xx=https_statistics->resp_3xx;
	rs_output->d_hfs_4xx=https_statistics->resp_4xx;
	rs_output->d_hfs_5xx=https_statistics->resp_5xx;
	rs_output->d_hfs_err=https_statistics->other_errs;
	rs_output->d_hfs_t_err=https_statistics->url_timeout_errs;
	rs_output->d_hfs_delay=https_statistics->appl_delay;
	rs_output->d_hfs_delay_2xx=https_statistics->appl_delay_2xx;
	rs_output->d_hfs_input_traffic=https_statistics->data_in/rs_output->d_cycle_interval;
	rs_output->d_hfs_output_traffic=https_statistics->data_out/rs_output->d_cycle_interval;	

	rs_output->d_total_duration=(int)(now - bctx->start_time)/ 1000;
	if (!rs_output->d_total_duration)
	{
		rs_output->d_total_duration=1;
	}
	
	rs_output->d_total_http_request_num=http_statistics_total->requests;
	rs_output->d_total_hf_1xx=http_statistics_total->resp_1xx;
	rs_output->d_total_hf_2xx=http_statistics_total->resp_2xx;
	rs_output->d_total_hf_3xx=http_statistics_total->resp_3xx;
	rs_output->d_total_hf_4xx=http_statistics_total->resp_4xx;
	rs_output->d_total_hf_5xx=http_statistics_total->resp_5xx;
	rs_output->d_total_hf_err=http_statistics_total->other_errs;
	rs_output->d_total_hf_t_err=http_statistics_total->url_timeout_errs;
	rs_output->d_average_hf_delay=http_statistics_total->appl_delay;
	rs_output->d_average_hf_delay_2xx=http_statistics_total->appl_delay_2xx;
	rs_output->d_average_hf_input_traffic=http_statistics_total->data_in/rs_output->d_total_duration;
	rs_output->d_average_hf_output_traffic=http_statistics_total->data_out/rs_output->d_total_duration;
	rs_output->d_total_https_request_num=https_statistics_total->requests;
	rs_output->d_total_hfs_1xx=https_statistics_total->resp_1xx;
	rs_output->d_total_hfs_2xx=https_statistics_total->resp_2xx;
	rs_output->d_total_hfs_3xx=https_statistics_total->resp_3xx;
	rs_output->d_total_hfs_4xx=https_statistics_total->resp_4xx;
	rs_output->d_total_hfs_5xx=https_statistics_total->resp_5xx;
	rs_output->d_total_hfs_err=https_statistics_total->other_errs;
	rs_output->d_total_hfs_t_err=https_statistics_total->url_timeout_errs;
	rs_output->d_average_hfs_delay=https_statistics_total->appl_delay;
	rs_output->d_average_hfs_delay_2xx=https_statistics_total->appl_delay_2xx;
	rs_output->d_average_hfs_input_traffic=https_statistics_total->data_in/rs_output->d_total_duration;
	rs_output->d_average_hfs_output_traffic=https_statistics_total->data_out/rs_output->d_total_duration;
	rs_output->d_average_caps=bctx->op_total.call_init_count/rs_output->d_total_duration;

	bctx->last_measure = now;
	stat_point_reset (&bctx->http_delta); 
	stat_point_reset (&bctx->https_delta);

	return;
}

int connect_redis_server(batch_context* bctx)
{
	struct timeval timeout = { 1, 500000 };   /* 1.5 seconds */
	redisReply *reply;

	bctx->redis_handle = redisConnectWithTimeout(bctx->redis_server, bctx->redis_port, timeout);
	if (bctx->redis_handle == NULL || bctx->redis_handle->err) 
	{
		if (bctx->redis_handle) 
		{
			printf("Connection error: %s\n", bctx->redis_handle->errstr);
			redisFree(bctx->redis_handle);

			return -1;
		} 
		else 
		{
			printf("Connection error: can't allocate redis context\n");
		}

		return -1;
	}

	/*Add the hostname to a set*/
	reply = redisCommand(bctx->redis_handle,"SADD hostname_set %s", bctx->cl_hostname);
	freeReplyObject(reply);

	return 0;
}

static void safe_free(char *s)
{
	if (s)
	{
		free(s);
		s=NULL;
	}
}

static void free_rs_output_str(redis_statistics_output *rs_output)
{
	safe_free (rs_output->s_redis_keepalive_secs);
	safe_free (rs_output->s_total_client);
	safe_free (rs_output->s_cycle_interval);
	safe_free (rs_output->s_http_request_num);
	safe_free (rs_output->s_hf_1xx);
	safe_free (rs_output->s_hf_2xx);
	safe_free (rs_output->s_hf_3xx);
	safe_free (rs_output->s_hf_4xx);
	safe_free (rs_output->s_hf_5xx);
	safe_free (rs_output->s_hf_err);
	safe_free (rs_output->s_hf_t_err);
	safe_free (rs_output->s_hf_delay);
	safe_free (rs_output->s_hf_delay_2xx);
	safe_free (rs_output->s_hf_input_traffic);
	safe_free (rs_output->s_hf_output_traffic);
	safe_free (rs_output->s_https_request_num);
	safe_free (rs_output->s_hfs_1xx);
	safe_free (rs_output->s_hfs_2xx);
	safe_free (rs_output->s_hfs_3xx);
	safe_free (rs_output->s_hfs_4xx);
	safe_free (rs_output->s_hfs_5xx);
	safe_free (rs_output->s_hfs_err);
	safe_free (rs_output->s_hfs_t_err);
	safe_free (rs_output->s_hfs_delay);
	safe_free (rs_output->s_hfs_delay_2xx);
	safe_free (rs_output->s_hfs_input_traffic);
	safe_free (rs_output->s_hfs_output_traffic);
	safe_free (rs_output->s_caps_current);
	safe_free (rs_output->s_total_duration);
	safe_free (rs_output->s_total_http_request_num);
	safe_free (rs_output->s_total_hf_1xx);
	safe_free (rs_output->s_total_hf_2xx);
	safe_free (rs_output->s_total_hf_3xx);
	safe_free (rs_output->s_total_hf_4xx);
	safe_free (rs_output->s_total_hf_5xx);
	safe_free (rs_output->s_total_hf_err);
	safe_free (rs_output->s_total_hf_t_err);
	safe_free (rs_output->s_average_hf_delay);
	safe_free (rs_output->s_average_hf_delay_2xx);
	safe_free (rs_output->s_average_hf_input_traffic);
	safe_free (rs_output->s_average_hf_output_traffic);
	safe_free (rs_output->s_total_https_request_num);
	safe_free (rs_output->s_total_hfs_1xx);
	safe_free (rs_output->s_total_hfs_2xx);
	safe_free (rs_output->s_total_hfs_3xx);
	safe_free (rs_output->s_total_hfs_4xx);
	safe_free (rs_output->s_total_hfs_5xx);
	safe_free (rs_output->s_total_hfs_err);
	safe_free (rs_output->s_total_hfs_t_err);
	safe_free (rs_output->s_average_hfs_delay);
	safe_free (rs_output->s_average_hfs_delay_2xx);
	safe_free (rs_output->s_average_hfs_input_traffic);
	safe_free (rs_output->s_average_hfs_output_traffic);
	safe_free (rs_output->s_average_caps);
}

int disconnect_redis_server (batch_context* bctx)
{
	if (bctx->redis_handle)
	{
		redisFree(bctx->redis_handle);
	}

	free_rs_output_str (&bctx->rs_output);

	return 0;
}

static void itoa(int d, char **s)
{
	if (*s == NULL)
	{
		*s=(char *)malloc (STATS_NUM_LENGTH_MAX);
		if (!(*s))
		{
			fprintf (stderr, "%s: malloc error.\n", __FUNCTION__);
			return;
		}		
	}
	memset(*s, 0, STATS_NUM_LENGTH_MAX);
	snprintf (*s, STATS_NUM_LENGTH_MAX, "%d", d);

	return;
}

static void rs_output_itoa(redis_statistics_output *rs_output)
{
	itoa(rs_output->d_redis_keepalive_secs, &rs_output->s_redis_keepalive_secs);
	itoa(rs_output->d_total_client, &rs_output->s_total_client);
	itoa(rs_output->d_cycle_interval, &rs_output->s_cycle_interval);
	itoa(rs_output->d_http_request_num, &rs_output->s_http_request_num);
	itoa(rs_output->d_hf_1xx, &rs_output->s_hf_1xx);
	itoa(rs_output->d_hf_2xx, &rs_output->s_hf_2xx);
	itoa(rs_output->d_hf_3xx, &rs_output->s_hf_3xx);
	itoa(rs_output->d_hf_4xx, &rs_output->s_hf_4xx);
	itoa(rs_output->d_hf_5xx, &rs_output->s_hf_5xx);
	itoa(rs_output->d_hf_err, &rs_output->s_hf_err);
	itoa(rs_output->d_hf_t_err, &rs_output->s_hf_t_err);
	itoa(rs_output->d_hf_delay, &rs_output->s_hf_delay);
	itoa(rs_output->d_hf_delay_2xx, &rs_output->s_hf_delay_2xx);
	itoa(rs_output->d_hf_input_traffic, &rs_output->s_hf_input_traffic);
	itoa(rs_output->d_hf_output_traffic, &rs_output->s_hf_output_traffic);
	itoa(rs_output->d_https_request_num, &rs_output->s_https_request_num);
	itoa(rs_output->d_hfs_1xx, &rs_output->s_hfs_1xx);
	itoa(rs_output->d_hfs_2xx, &rs_output->s_hfs_2xx);
	itoa(rs_output->d_hfs_3xx, &rs_output->s_hfs_3xx);
	itoa(rs_output->d_hfs_4xx, &rs_output->s_hfs_4xx);
	itoa(rs_output->d_hfs_5xx, &rs_output->s_hfs_5xx);
	itoa(rs_output->d_hfs_err, &rs_output->s_hfs_err);
	itoa(rs_output->d_hfs_t_err, &rs_output->s_hfs_t_err);
	itoa(rs_output->d_hfs_delay, &rs_output->s_hfs_delay);
	itoa(rs_output->d_hfs_delay_2xx, &rs_output->s_hfs_delay_2xx);
	itoa(rs_output->d_hfs_input_traffic, &rs_output->s_hfs_input_traffic);
	itoa(rs_output->d_hfs_output_traffic, &rs_output->s_hfs_output_traffic);
	itoa(rs_output->d_caps_current, &rs_output->s_caps_current);
	itoa(rs_output->d_total_duration, &rs_output->s_total_duration);
	itoa(rs_output->d_total_http_request_num, &rs_output->s_total_http_request_num);
	itoa(rs_output->d_total_hf_1xx, &rs_output->s_total_hf_1xx);
	itoa(rs_output->d_total_hf_2xx, &rs_output->s_total_hf_2xx);
	itoa(rs_output->d_total_hf_3xx, &rs_output->s_total_hf_3xx);
	itoa(rs_output->d_total_hf_4xx, &rs_output->s_total_hf_4xx);
	itoa(rs_output->d_total_hf_5xx, &rs_output->s_total_hf_5xx);
	itoa(rs_output->d_total_hf_err, &rs_output->s_total_hf_err);
	itoa(rs_output->d_total_hf_t_err, &rs_output->s_total_hf_t_err);
	itoa(rs_output->d_average_hf_delay, &rs_output->s_average_hf_delay);
	itoa(rs_output->d_average_hf_delay_2xx, &rs_output->s_average_hf_delay_2xx);
	itoa(rs_output->d_average_hf_input_traffic, &rs_output->s_average_hf_input_traffic);
	itoa(rs_output->d_average_hf_output_traffic, &rs_output->s_average_hf_output_traffic);
	itoa(rs_output->d_total_https_request_num, &rs_output->s_total_https_request_num);
	itoa(rs_output->d_total_hfs_1xx, &rs_output->s_total_hfs_1xx);
	itoa(rs_output->d_total_hfs_2xx, &rs_output->s_total_hfs_2xx);
	itoa(rs_output->d_total_hfs_3xx, &rs_output->s_total_hfs_3xx);
	itoa(rs_output->d_total_hfs_4xx, &rs_output->s_total_hfs_4xx);
	itoa(rs_output->d_total_hfs_5xx, &rs_output->s_total_hfs_5xx);
	itoa(rs_output->d_total_hfs_err, &rs_output->s_total_hfs_err);
	itoa(rs_output->d_total_hfs_t_err, &rs_output->s_total_hfs_t_err);
	itoa(rs_output->d_average_hfs_delay, &rs_output->s_average_hfs_delay);
	itoa(rs_output->d_average_hfs_delay_2xx, &rs_output->s_average_hfs_delay_2xx);
	itoa(rs_output->d_average_hfs_input_traffic, &rs_output->s_average_hfs_input_traffic);
	itoa(rs_output->d_average_hfs_output_traffic, &rs_output->s_average_hfs_output_traffic);
	itoa(rs_output->d_average_caps, &rs_output->s_average_caps);
}

static void send_statistics_to_redis (batch_context* bctx)
{
	redis_statistics_output *rs_output=&(bctx->rs_output);
	redisReply *reply;;

	assert (bctx->redis_handle != NULL);
	rs_output_itoa (rs_output);

	reply = redisCommand(bctx->redis_handle, "SETEX %s_ttl %s %s", bctx->cl_hostname, rs_output->s_redis_keepalive_secs, rs_output->s_redis_keepalive_secs);
	freeReplyObject(reply);

	reply = redisCommand(bctx->redis_handle, "HMSET %s batch_name %s total_client %s cycle_interval %s hf_request_num %s hfs_request_num %s caps_current %s  \
        hf_1xx %s hf_2xx %s hf_3xx %s hf_4xx %s hf_5xx %s hf_err %s hf_t_err %s hf_delay %s hf_delay_2xx %s hf_input_traffic %s hf_output_traffic %s  \
        hfs_1xx %s hfs_2xx %s hfs_3xx %s hfs_4xx %s hfs_5xx %s hfs_err %s hfs_t_err %s hfs_delay %s hfs_delay_2xx %s hfs_input_traffic %s hfs_output_traffic %s  \
        total_duration %s total_hf_request_num %s total_hfs_request_num %s average_caps %s  \
        total_hf_1xx %s total_hf_2xx %s total_hf_3xx %s total_hf_4xx %s total_hf_5xx %s   \
        total_hf_err %s total_hf_t_err %s average_hf_delay %s average_hf_delay_2xx %s average_hf_input_traffic %s average_hf_output_traffic %s   \
        total_hfs_1xx %s total_hfs_2xx %s total_hfs_3xx %s total_hfs_4xx %s total_hfs_5xx %s  \
        total_hfs_err %s total_hfs_t_err %s average_hfs_delay %s average_hfs_delay_2xx %s average_hfs_input_traffic %s average_hfs_output_traffic %s",
		bctx->cl_hostname, bctx->batch_name, rs_output->s_total_client, rs_output->s_cycle_interval, rs_output->s_http_request_num, rs_output->s_https_request_num, rs_output->s_caps_current,
		rs_output->s_hf_1xx, rs_output->s_hf_2xx, rs_output->s_hf_3xx, rs_output->s_hf_4xx, rs_output->s_hf_5xx, 
		   rs_output->s_hf_err, rs_output->s_hf_t_err, rs_output->s_hf_delay, rs_output->s_hf_delay_2xx, rs_output->s_hf_input_traffic, rs_output->s_hf_output_traffic,
		rs_output->s_hfs_1xx, rs_output->s_hfs_2xx, rs_output->s_hfs_3xx, rs_output->s_hfs_4xx, rs_output->s_hfs_5xx, 
		    rs_output->s_hfs_err, rs_output->s_hfs_t_err, rs_output->s_hfs_delay, rs_output->s_hfs_delay_2xx, rs_output->s_hfs_input_traffic, rs_output->s_hfs_output_traffic,
		rs_output->s_total_duration, rs_output->s_total_http_request_num, rs_output->s_total_https_request_num, rs_output->s_average_caps,
		rs_output->s_total_hf_1xx, rs_output->s_total_hf_2xx, rs_output->s_total_hf_3xx, rs_output->s_total_hf_4xx, rs_output->s_total_hf_5xx, 
		rs_output->s_total_hf_err, rs_output->s_total_hf_t_err, rs_output->s_average_hf_delay, rs_output->s_average_hf_delay_2xx, 
		    rs_output->s_average_hf_input_traffic, rs_output->s_average_hf_output_traffic,
		rs_output->s_total_hfs_1xx, rs_output->s_total_hfs_2xx, rs_output->s_total_hfs_3xx, rs_output->s_total_hfs_4xx, rs_output->s_total_hfs_5xx, 
		rs_output->s_total_hfs_err, rs_output->s_total_hfs_t_err, rs_output->s_average_hfs_delay, rs_output->s_average_hfs_delay_2xx, 
		    rs_output->s_average_hfs_input_traffic, rs_output->s_average_hfs_output_traffic);
	freeReplyObject(reply);

#if 0
	printf ("HMSET %s batch_name %s total_client %s cycle_interval %s http_request_num %s https_request_num %s caps_current %s  \
		hf_1xx %s hf_2xx %s hf_3xx %s hf_4xx %s hf_5xx %s hf_err %s hf_t_err %s hf_delay %s hf_delay_2xx %s hf_input_traffic %s hf_output_traffic %s  \
		hfs_1xx %s hfs_2xx %s hfs_3xx %s hfs_4xx %s hfs_5xx %s hfs_err %s hfs_t_err %s hfs_delay %s hfs_delay_2xx %s hfs_input_traffic %s hfs_output_traffic %s  \
		total_duration %s total_http_request_num %s total_https_request_num %s average_caps %s  \
		total_hf_1xx %s total_hf_2xx %s total_hf_3xx %s total_hf_4xx %s total_hf_5xx %s   \
		total_hf_err %s total_hf_t_err %s average_hf_delay %s average_hf_delay_2xx %s average_hf_input_traffic %s average_hf_output_traffic %s   \
		total_hfs_1xx %s total_hfs_2xx %s total_hfs_3xx %s total_hfs_4xx %s total_hfs_5xx %s  \
		total_hfs_err %s total_hfs_t_err %s average_hfs_delay %s average_hfs_delay_2xx %s average_hfs_input_traffic %s average_hfs_output_traffic %s\n",
		bctx->cl_hostname, bctx->batch_name, rs_output->s_total_client, rs_output->s_cycle_interval, rs_output->s_http_request_num, rs_output->s_https_request_num, rs_output->s_caps_current,
		rs_output->s_hf_1xx, rs_output->s_hf_2xx, rs_output->s_hf_3xx, rs_output->s_hf_4xx, rs_output->s_hf_5xx, 
		   rs_output->s_hf_err, rs_output->s_hf_t_err, rs_output->s_hf_delay, rs_output->s_hf_delay_2xx, rs_output->s_hf_input_traffic, rs_output->s_hf_output_traffic,
		rs_output->s_hfs_1xx, rs_output->s_hfs_2xx, rs_output->s_hfs_3xx, rs_output->s_hfs_4xx, rs_output->s_hfs_5xx, 
		    rs_output->s_hfs_err, rs_output->s_hfs_t_err, rs_output->s_hfs_delay, rs_output->s_hfs_delay_2xx, rs_output->s_hfs_input_traffic, rs_output->s_hfs_output_traffic,
		rs_output->s_total_duration, rs_output->s_total_http_request_num, rs_output->s_total_https_request_num, rs_output->s_average_caps,
		rs_output->s_total_hf_1xx, rs_output->s_total_hf_2xx, rs_output->s_total_hf_3xx, rs_output->s_total_hf_4xx, rs_output->s_total_hf_5xx, 
		rs_output->s_total_hf_err, rs_output->s_total_hf_t_err, rs_output->s_average_hf_delay, rs_output->s_average_hf_delay_2xx, 
		    rs_output->s_average_hf_input_traffic, rs_output->s_average_hf_output_traffic,
		rs_output->s_total_hfs_1xx, rs_output->s_total_hfs_2xx, rs_output->s_total_hfs_3xx, rs_output->s_total_hfs_4xx, rs_output->s_total_hfs_5xx, 
		rs_output->s_total_hfs_err, rs_output->s_total_hfs_t_err, rs_output->s_average_hfs_delay, rs_output->s_average_hfs_delay_2xx, 
		    rs_output->s_average_hfs_input_traffic, rs_output->s_average_hfs_output_traffic);

	reply = redisCommand(bctx->redis_handle, "HMSET %s batch_name %s total_client %s cycle_interval %s http_request_num %s https_request_num %s caps_current %s",
		bctx->cl_hostname, bctx->batch_name, rs_output->s_total_client, rs_output->s_cycle_interval, rs_output->s_http_request_num, 
		rs_output->s_https_request_num, rs_output->s_caps_current);
	freeReplyObject(reply);

	reply = redisCommand(bctx->redis_handle, "HMSET %s hf_1xx %s hf_2xx %s hf_3xx %s hf_4xx %s hf_5xx %s \
		hf_err %s hf_t_err %s hf_delay %s hf_delay_2xx %s hf_input_traffic %s hf_output_traffic %s",
		bctx->cl_hostname, rs_output->s_hf_1xx, rs_output->s_hf_2xx, rs_output->s_hf_3xx, rs_output->s_hf_4xx, rs_output->s_hf_5xx, 
		rs_output->s_hf_err, rs_output->s_hf_t_err, rs_output->s_hf_delay, rs_output->s_hf_delay_2xx, rs_output->s_hf_input_traffic, rs_output->s_hf_output_traffic);
	freeReplyObject(reply);
	
	reply = redisCommand(bctx->redis_handle, "HMSET %s hfs_1xx %s hfs_2xx %s hfs_3xx %s hfs_4xx %s hfs_5xx %s  \
		hfs_err %s hfs_t_err %s hfs_delay %s hfs_delay_2xx %s hfs_input_traffic %s hfs_output_traffic %s",
		bctx->cl_hostname, rs_output->s_hfs_1xx, rs_output->s_hfs_2xx, rs_output->s_hfs_3xx, rs_output->s_hfs_4xx, rs_output->s_hfs_5xx, 
		rs_output->s_hfs_err, rs_output->s_hfs_t_err, rs_output->s_hfs_delay, rs_output->s_hfs_delay_2xx, rs_output->s_hfs_input_traffic, rs_output->s_hfs_output_traffic);
	freeReplyObject(reply);
	
	reply = redisCommand(bctx->redis_handle, "HMSET %s total_duration %s total_http_request_num %s total_https_request_num %s average_caps %s", 
		bctx->cl_hostname, rs_output->s_total_duration, rs_output->s_total_http_request_num, rs_output->s_total_https_request_num, rs_output->s_average_caps);
	freeReplyObject(reply);
	
	reply = redisCommand(bctx->redis_handle, "HMSET %s total_hf_1xx %s total_hf_2xx %s total_hf_3xx %s total_hf_4xx %s total_hf_5xx %s  \
		total_hf_err %s total_hf_t_err %s average_hf_delay %s average_hf_delay_2xx %s average_hf_input_traffic %s average_hf_output_traffic %s", 
		rs_output->s_total_hf_1xx, rs_output->s_total_hf_2xx, rs_output->s_total_hf_3xx, rs_output->s_total_hf_4xx, rs_output->s_total_hf_5xx, 
		rs_output->s_total_hf_err, rs_output->s_total_hf_t_err, rs_output->s_average_hf_delay, rs_output->s_average_hf_delay_2xx,
		rs_output->s_average_hf_input_traffic, rs_output->s_average_hf_output_traffic);
	freeReplyObject(reply);
	
	reply = redisCommand(bctx->redis_handle, "HMSET %s total_hfs_1xx %s total_hfs_2xx %s total_hfs_3xx %s total_hfs_4xx %s total_hfs_5xx %s  \
		total_hfs_err %s total_hfs_t_err %s average_hfs_delay %s average_hfs_delay_2xx %s average_hfs_input_traffic %s average_hfs_output_traffic %s", 
		rs_output->s_total_hfs_1xx, rs_output->s_total_hfs_2xx, rs_output->s_total_hfs_3xx, rs_output->s_total_hfs_4xx, rs_output->s_total_hfs_5xx, 
		rs_output->s_total_hfs_err, rs_output->s_total_hfs_t_err, rs_output->s_average_hfs_delay, rs_output->s_average_hfs_delay_2xx,
		rs_output->s_average_hfs_input_traffic, rs_output->s_average_hfs_output_traffic);
	freeReplyObject(reply);

#endif 
}

static void print_statistics_to_screen(batch_context* bctx)
{
	redis_statistics_output *rs_output=&(bctx->rs_output);
	
	fprintf(stdout,"=======================================  loading batch is: %-10.10s  =======================================\n",bctx->batch_name);
	fprintf(stdout,"------------------------------------------------------------------------------------------------------------\n");
	fprintf(stdout,"Cycle statistics (Cycle interval:%d sec, clients:%d, CAPS-current:%d):\n", rs_output->d_cycle_interval, rs_output->d_total_client, rs_output->d_caps_current);
	fprintf(stdout, "H/F    Req:%d,1xx:%d,2xx:%d,3xx:%d,4xx:%d,5xx:%d,Err:%d,T-Err:%d,D:%dms,D-2xx:%dms,Ti:%dB/s,To:%dB/s\n",
	    rs_output->d_http_request_num, rs_output->d_hf_1xx, rs_output->d_hf_2xx, rs_output->d_hf_3xx, rs_output->d_hf_4xx, rs_output->d_hf_5xx, 
	    rs_output->d_hf_err, rs_output->d_hf_t_err, rs_output->d_hf_delay, rs_output->d_hf_delay_2xx, rs_output->d_hf_input_traffic, rs_output->d_hf_output_traffic);
	fprintf(stdout, "H/F/s  Req:%d,1xx:%d,2xx:%d,3xx:%d,4xx:%d,5xx:%d,Err:%d,T-Err:%d,D:%dms,D-2xx:%dms,Ti:%dB/s,To:%dB/s\n",
	    rs_output->d_https_request_num, rs_output->d_hfs_1xx, rs_output->d_hfs_2xx, rs_output->d_hfs_3xx, rs_output->d_hfs_4xx, rs_output->d_hfs_5xx, 
	    rs_output->d_hfs_err, rs_output->d_hfs_t_err, rs_output->d_hfs_delay, rs_output->d_hfs_delay_2xx, rs_output->d_hfs_input_traffic, rs_output->d_hfs_output_traffic);
	fprintf(stdout,"------------------------------------------------------------------------------------------------------------\n");
	fprintf(stdout,"Summary statistics (Summay interval:%d secs, CAPS-average:%d):\n", rs_output->d_total_duration, rs_output->d_average_caps);
	fprintf(stdout, "H/F    Req:%d,1xx:%d,2xx:%d,3xx:%d,4xx:%d,5xx:%d,Err:%d,T-Err:%d,D:%dms,D-2xx:%dms,Ti:%dB/s,To:%dB/s\n",
	    rs_output->d_total_http_request_num, rs_output->d_total_hf_1xx, rs_output->d_total_hf_2xx, rs_output->d_total_hf_3xx, rs_output->d_total_hf_4xx, rs_output->d_total_hf_5xx, 
	    rs_output->d_total_hf_err, rs_output->d_total_hf_t_err, rs_output->d_average_hf_delay, rs_output->d_average_hf_delay_2xx, rs_output->d_average_hf_input_traffic, rs_output->d_average_hf_output_traffic);
	fprintf(stdout, "H/F/s  Req:%d,1xx:%d,2xx:%d,3xx:%d,4xx:%d,5xx:%d,Err:%d,T-Err:%d,D:%dms,D-2xx:%dms,Ti:%dB/s,To:%dB/s\n",
	    rs_output->d_total_https_request_num, rs_output->d_total_hfs_1xx, rs_output->d_total_hfs_2xx, rs_output->d_total_hfs_3xx, rs_output->d_total_hfs_4xx, rs_output->d_total_hfs_5xx, 
	    rs_output->d_total_hfs_err, rs_output->d_total_hfs_t_err, rs_output->d_average_hfs_delay, rs_output->d_average_hfs_delay_2xx, rs_output->d_average_hfs_input_traffic, rs_output->d_average_hfs_output_traffic);
	fprintf(stdout,"==============================================================================================================\n");
    fflush (stdout);

	return;
}


static void generate_final_reporting (batch_context* bctx, unsigned long now)
{
	redis_statistics_output *rs_output=&(bctx->rs_output);
	
	prepare_reporting_data (bctx, now);
	if (bctx->reporting_way == CONSOLE_REPORTING || bctx->reporting_way == CONSOLE_AND_REDIS)
	{
		fprintf(stdout, "H/F    Req:%d,1xx:%d,2xx:%d,3xx:%d,4xx:%d,5xx:%d,Err:%d,T-Err:%d,D:%dms,D-2xx:%dms,Ti:%dB/s,To:%dB/s\n",
	    	rs_output->d_http_request_num, rs_output->d_hf_1xx, rs_output->d_hf_2xx, rs_output->d_hf_3xx, rs_output->d_hf_4xx, rs_output->d_hf_5xx, 
	    	rs_output->d_hf_err, rs_output->d_hf_t_err, rs_output->d_hf_delay, rs_output->d_hf_delay_2xx, rs_output->d_hf_input_traffic, rs_output->d_hf_output_traffic);
		fprintf(stdout, "H/F/s  Req:%d,1xx:%d,2xx:%d,3xx:%d,4xx:%d,5xx:%d,Err:%d,T-Err:%d,D:%dms,D-2xx:%dms,Ti:%dB/s,To:%dB/s\n",
	    	rs_output->d_https_request_num, rs_output->d_hfs_1xx, rs_output->d_hfs_2xx, rs_output->d_hfs_3xx, rs_output->d_hfs_4xx, rs_output->d_hfs_5xx, 
	    	rs_output->d_hfs_err, rs_output->d_hfs_t_err, rs_output->d_hfs_delay, rs_output->d_hfs_delay_2xx, rs_output->d_hfs_input_traffic, rs_output->d_hfs_output_traffic);
		fprintf(stdout,"==============================================================================================================\n");
		fprintf(stdout,"End of the test for batch: %-10.10s\n", bctx->batch_name); 
		fprintf(stdout,"==============================================================================================================\n");
		fprintf(stdout,"\nTest total duration is %d seconds and CAPS average is %d:\n", rs_output->d_total_duration, rs_output->d_average_caps);
		fprintf(stdout, "H/F     Req:%d,1xx:%d,2xx:%d,3xx:%d,4xx:%d,5xx:%d,Err:%d,T-Err:%d,D:%dms,D-2xx:%dms,Ti:%dB/s,To:%dB/s\n",
	        rs_output->d_total_http_request_num, rs_output->d_total_hf_1xx, rs_output->d_total_hf_2xx, rs_output->d_total_hf_3xx, rs_output->d_total_hf_4xx, rs_output->d_total_hf_5xx, 
	        rs_output->d_total_hf_err, rs_output->d_total_hf_t_err, 
	        rs_output->d_average_hf_delay, rs_output->d_average_hf_delay_2xx, rs_output->d_average_hf_input_traffic, rs_output->d_average_hf_output_traffic);
		fprintf(stdout, "H/F/s	 Req:%d,1xx:%d,2xx:%d,3xx:%d,4xx:%d,5xx:%d,Err:%d,T-Err:%d,D:%dms,D-2xx:%dms,Ti:%dB/s,To:%dB/s\n",
	        rs_output->d_total_https_request_num, rs_output->d_total_hfs_1xx, rs_output->d_total_hfs_2xx, rs_output->d_total_hfs_3xx, rs_output->d_total_hfs_4xx, rs_output->d_total_hfs_5xx, 
	        rs_output->d_total_hfs_err, rs_output->d_total_hfs_t_err, 
	        rs_output->d_average_hfs_delay, rs_output->d_average_hfs_delay_2xx, rs_output->d_average_hfs_input_traffic, rs_output->d_average_hfs_output_traffic);
	}
	
	if (bctx->reporting_way == REDIS_REPORTING || bctx->reporting_way == CONSOLE_AND_REDIS)
	{
		send_statistics_to_redis(bctx);
	}

    generate_final_log(bctx);
	
	return;
}

void generate_cycle_reporting(batch_context* bctx, unsigned long now)
{
	if (!stop_loading)
	{
		fprintf(stdout, "\033[2J");
	}

	if (stop_loading)
	{
		generate_final_reporting (bctx, now);
		//screen_release ();
		exit (1); 
	}

	
	prepare_reporting_data (bctx, now);
	if (bctx->reporting_way == CONSOLE_REPORTING)
	{
		print_statistics_to_screen(bctx);
	}
	else if (bctx->reporting_way == REDIS_REPORTING)
	{
		send_statistics_to_redis(bctx);
	}
	else if (bctx->reporting_way == CONSOLE_AND_REDIS)
	{
		send_statistics_to_redis(bctx);
		print_statistics_to_screen(bctx);
	}
	
	generate_cycle_log(bctx,now,(bctx->rs_output).d_total_client);

	return;
}

