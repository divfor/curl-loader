/*
*     statistics.h
*
* 2006-2007 Copyright (c) 
* Robert Iakobashvili, <coroberti@gmail.com>
* Michael Moser,  <moser.michael@gmail.com>                 
* All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdio.h>

#include "timer_tick.h"

typedef enum reporting_way{
	CONSOLE_REPORTING=0,
	REDIS_REPORTING,
	CONSOLE_AND_REDIS,
}reporting_way;


/*
  stat_point -the structure is used to collect loading statistics.
  Two instances of the structure are kept by each batch context. 
  One object is used for the latest snapshot interval stats and another
  for the total summary values.
*/
typedef struct stat_point
{
   /* Inbound bytes number */
  unsigned long long data_in;
   /* Outbound bytes number */
  unsigned long long data_out;

   /* Number of requests received */
  unsigned long requests;

  /* Number of 1xx responses */
  unsigned long resp_1xx;

  /* Number of 2xx responses */
  unsigned long resp_2xx;

  /* Number of 3xx redirections */
  unsigned long resp_3xx;

  /* Number of 4xx responses */
  unsigned long resp_4xx;

  /* Number of 5xx responses */
  unsigned long resp_5xx;

  /* Errors of resolving, connecting, internal errors, etc. */
  unsigned long other_errs;

  /* 
     URL Timeout errors of not accomplishing url fetch prior to the 
     url-completion timer being expired .
  */
  unsigned long url_timeout_errs;

   /* Num of data points used to calculate average application delay */
  int appl_delay_points;
  /* Average delay in msec between request and response */
  unsigned long  appl_delay;

  /* 
     Num of data points used to calculate average application delay 
     for 2xx-OK responses.
  */
  int appl_delay_2xx_points;
   /* Average delay in msec between request and 2xx-OK response */
  unsigned long  appl_delay_2xx;

} stat_point;

/*
  op_stat_point - operation statistics point.
  Two instances are residing in each batch context and used:
  - one for the latest snapshot interval;
  - another for the total summary values.
*/
typedef struct op_stat_point
{
  /* Number of url-counters in the below arrays */
  unsigned long url_num;

  /* Array of url counters for successful fetches */
  unsigned long* url_ok;

  /* Array of url counters for failed fetches */
  unsigned long* url_failed;

  /* Array of url counters for timeouted fetches */
  unsigned long* url_timeouted;

  /* Used for CAPS calculation */
  unsigned long call_init_count;

} op_stat_point;


typedef struct redis_statistics_output
{
	int d_redis_keepalive_secs;
	char *s_redis_keepalive_secs;
	
	int d_total_client;
	char *s_total_client;

	int d_cycle_interval;
	char *s_cycle_interval;

	int d_http_request_num;
	char *s_http_request_num;

	int d_hf_1xx;
	char *s_hf_1xx;

	int d_hf_2xx;
	char *s_hf_2xx;

	int d_hf_3xx;
	char *s_hf_3xx;

	int d_hf_4xx;
	char *s_hf_4xx;

	int d_hf_5xx;
	char *s_hf_5xx;

	int d_hf_err;
	char *s_hf_err;

	int d_hf_t_err;
	char *s_hf_t_err;

	int d_hf_delay;
	char *s_hf_delay;
		
	int d_hf_delay_2xx;
	char *s_hf_delay_2xx;

	int d_hf_input_traffic;
	char *s_hf_input_traffic;

	int d_hf_output_traffic;
	char *s_hf_output_traffic;

	int d_https_request_num;
	char *s_https_request_num;

	int d_hfs_1xx;
	char *s_hfs_1xx;

	int d_hfs_2xx;
	char *s_hfs_2xx;

	int d_hfs_3xx;
	char *s_hfs_3xx;

	int d_hfs_4xx;
	char *s_hfs_4xx;

	int d_hfs_5xx;
	char *s_hfs_5xx;

	int d_hfs_err;
	char *s_hfs_err;

	int d_hfs_t_err;
	char *s_hfs_t_err;

	int d_hfs_delay;
	char *s_hfs_delay;

	int d_hfs_delay_2xx;
	char *s_hfs_delay_2xx;

	int d_hfs_input_traffic;
	char *s_hfs_input_traffic;

	int d_hfs_output_traffic;
	char *s_hfs_output_traffic;

	int d_caps_current;
	char *s_caps_current;

	int d_total_duration;
	char *s_total_duration;

	int d_total_http_request_num;
	char *s_total_http_request_num;
		 
	int d_total_hf_1xx;
	char *s_total_hf_1xx;

	int d_total_hf_2xx;
	char *s_total_hf_2xx;

	int d_total_hf_3xx;
	char *s_total_hf_3xx;

	int d_total_hf_4xx;
	char *s_total_hf_4xx;

	int d_total_hf_5xx;
	char *s_total_hf_5xx;

	int d_total_hf_err;
	char *s_total_hf_err;

	int d_total_hf_t_err;
	char *s_total_hf_t_err;

	int d_average_hf_delay;
	char *s_average_hf_delay;

	int d_average_hf_delay_2xx;
	char *s_average_hf_delay_2xx;

	int d_average_hf_input_traffic;
	char *s_average_hf_input_traffic;

	int d_average_hf_output_traffic;
	char *s_average_hf_output_traffic;

	int d_total_https_request_num;
	char *s_total_https_request_num;

	int d_total_hfs_1xx;
	char *s_total_hfs_1xx;

	int d_total_hfs_2xx;
	char *s_total_hfs_2xx;

	int d_total_hfs_3xx;
	char *s_total_hfs_3xx;

	int d_total_hfs_4xx;
	char *s_total_hfs_4xx;

	int d_total_hfs_5xx;
	char *s_total_hfs_5xx;

	int d_total_hfs_err;
	char *s_total_hfs_err;

	int d_total_hfs_t_err;
	char *s_total_hfs_t_err;

	int d_average_hfs_delay;
	char *s_average_hfs_delay;

	int d_average_hfs_delay_2xx;
	char *s_average_hfs_delay_2xx;

	int d_average_hfs_input_traffic;
	char *s_average_hfs_input_traffic;

	int d_average_hfs_output_traffic;
	char *s_average_hfs_output_traffic;

	int d_average_caps;
	char *s_average_caps;
}redis_statistics_output;




/*******************************************************************************
* Function name - stat_point_add
*
* Description - Adds counters of one stat_point object to another
* Input -       *left  - pointer to the stat_point, where counter will be added
*               *right - pointer to the stat_point, which counter will be added 
*                        to the <left>
* Return Code/Output - None
********************************************************************************/
void stat_point_add (stat_point* left, stat_point* right);

/******************************************************************************
* Function name - stat_point_reset
*
* Description - Nulls counters of a stat_point structure
* 
* Input -       *point -  pointer to the stat_point
* Return Code/Output - None
*******************************************************************************/
void stat_point_reset (stat_point* point);


/*******************************************************************************
* Function name - op_stat_point_add
*
* Description - Adds counters of one op_stat_point object to another
* Input -       *left  -  pointer to the op_stat_point, where counter will be added
*               *right -  pointer to the op_stat_point, which counter will be 
*                         added to the <left>
* Return Code/Output - None
********************************************************************************/
void op_stat_point_add (op_stat_point* left, op_stat_point* right);


/*******************************************************************************
* Function name - op_stat_point_reset
*
* Description - Nulls counters of an op_stat_point structure
*
* Input -       *point -  pointer to the op_stat_point
* Return Code/Output - None
********************************************************************************/
void op_stat_point_reset (op_stat_point* point);


/*******************************************************************************
* Function name - op_stat_point_init
*
* Description - Initializes an allocated op_stat_point by allocating relevant 
*               pointer fields for counters
*
* Input -       *point  -pointer to the op_stat_point, where counter will be added
*               url_num -number of urls
* Return Code/Output - None
********************************************************************************/
int op_stat_point_init (op_stat_point* point, size_t url_num);


/*******************************************************************************
* Function name -  op_stat_point_release
*
* Description - Releases memory allocated by op_stat_point_init ()
* 
* Input -       *point -  pointer to the op_stat_point, where counter will be added
* Return Code/Output - None
********************************************************************************/
void op_stat_point_release (op_stat_point* point);

/*******************************************************************************
* Function name -  op_stat_update
*
* Description - Updates operation statistics using information from client context
*
* Input -       *point             - pointer to the op_stat_point, where counters 
* 			             to be updated
*               current_state      - current state of a client
*               prev_state         - previous state of a client
*               current_url_index  - current url index of a the client
*               prev_uas_url_index - previous url index of a the client
* Return Code/Output - None
*********************************************************************************/
void op_stat_update (op_stat_point* op_stat, 
                     int current_state, 
                     int prev_state,
                     size_t current_url_index,
                     size_t prev_url_index);

void op_stat_timeouted (op_stat_point* op_stat, size_t url_index);

void op_stat_call_init_count_inc (op_stat_point* op_stat);

struct client_context;
struct batch_context;

char *ascii_time (char *tbuf);
void print_statistics_header (FILE* file);

extern void generate_cycle_log (struct batch_context* bctx, unsigned long now_time, int clients_total_num);
extern void generate_final_log (struct batch_context* bctx);
extern int connect_redis_server(struct batch_context* bctx);
extern void generate_cycle_reporting(struct batch_context* bctx, unsigned long now);
extern int disconnect_redis_server (struct batch_context* bctx);
#endif /* STATISTICS_H */
