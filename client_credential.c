
// must be the first include
#include "fdsetsize.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <curl/curl.h>
#include <curl/multi.h>

#include "url.h"
#include "batch.h"
#include "client.h"
#include "loader.h"
#include "conf.h"

#define CLIENT_CRED_LENGTH_MIN 5
#define CLIENT_CRED_LENGTH_MAX 128

#define CREDENTIALS_SINGLE "SINGLE_USER"
#define CREDENTIALS_FROM_FILE "RECORDS_FROM_FILE"
#define CLIENT_CRED_NUM_MAX 50000

int client_creds_num=1;
int is_transparent_proxy=1;

static int proxy_credentials_type=0;   /*single type, default*/
authentication_method  proxy_auth_method=CURLAUTH_NONE;
static char** client_creds_array=NULL;

static int access_debug_flag=0;


/*Alloc the memory space and store the client_credentials as a global array*/
static int alloc_and_store_credentials(char* client_credentials, int index)
{
	int i=0;

	/*Alloc the space at the first time(index=0). After that, don't need to do that again*/
	if (index==0)
	{
	    client_creds_array=(char**)malloc(client_creds_num*sizeof(char *));
		for(i=0; i<client_creds_num; i++)
		{
		    client_creds_array[i]=(char*)malloc(CLIENT_CRED_LENGTH_MAX);
			if(!client_creds_array[i])
			{
			    printf("%s, malloc error.\n", __FUNCTION__);		  
				return -1;
			}

			memset (client_creds_array[i], 0, CLIENT_CRED_LENGTH_MAX);
		}
	}

	strncpy(client_creds_array[index], client_credentials, CLIENT_CRED_LENGTH_MAX);

    /*Deliminated the return char*/
	if ((client_creds_array[index][strlen(client_creds_array[index])-1]=='\r')
			||(client_creds_array[index][strlen(client_creds_array[index])-1]=='\n'))
	{
		client_creds_array[index][strlen(client_creds_array[index])-1]='\0';
	}

	return 0;
}

/*After bind the credentials with the client when initial the client context, we need to free the memory space*/
int free_client_credentials()
{
    int i=0;

    if (!client_creds_array)
	{
		return -1;
	}
	
	for(i=0; i<client_creds_num; i++)
	{
		if(client_creds_array[i])
		{
			free(client_creds_array[i]);
			client_creds_array[i]=NULL;
		}
	}
    
	free(client_creds_array);
	client_creds_array=NULL;

    return 0;
}


/*Read the credentials from the config file*/
int load_creds_from_file(char* filename)
{
    struct stat statbuf;
	char one_line[128]={0};
    FILE* fp;
	int i=0;

    if (stat (filename, &statbuf) == -1)
    {
        fprintf (stderr, "File %s does not exist.\n", filename);
        return -1;
    }

    if (!(fp = fopen(filename, "r")))
    {
        fprintf (stderr, "Open file %s failed.\n", filename);
        return -1;
    }

    client_creds_num=0;
	while(client_creds_num<CLIENT_CRED_NUM_MAX&&!feof(fp))
	{

		fgets(one_line, CLIENT_CRED_LENGTH_MAX, fp);
		if (strlen(one_line)<CLIENT_CRED_LENGTH_MIN)
		{
			continue;
		}
	
		client_creds_num++;
	}
    client_creds_num--;

    fseek(fp,0,SEEK_SET);

	memset(&one_line[0], 0, CLIENT_CRED_LENGTH_MAX);
	while(i<client_creds_num&&!feof(fp))
	{
		fgets(one_line, CLIENT_CRED_LENGTH_MAX, fp);
		if (strlen(one_line)<CLIENT_CRED_LENGTH_MIN)
		{
			continue;
		}

		alloc_and_store_credentials(one_line, i);
		i++;
	} 

	fclose(fp);

	return 0;
}

/*Is it using a single credential or multi credentials from a file?*/
int credentials_type_parser(batch_context*const bctx, char*const value)
{
	if (!bctx)
	{
		return -1;
	}

	if (!strcmp (value, CREDENTIALS_SINGLE))
	{
		proxy_credentials_type= 0;
	}
	else if (!strcmp (value, CREDENTIALS_FROM_FILE))
	{
		proxy_credentials_type = 1;
	}
	else
	{
		fprintf(stderr, "%s - error: CREDENTIALS_TYPE to be choosen from:"
				"%s , %s ,\n" ,  __func__, 
				CREDENTIALS_SINGLE, CREDENTIALS_FROM_FILE);
				return -1;
	}


    return 0;
}


int credentials_record_file_parser(batch_context*const bctx, char*const value)
{
	if (load_creds_from_file (value) == -1)
	{
		fprintf(stderr, "%s error: credentials_record_file_parser () failed.  %p\n", __func__, bctx);
		return -1;
	}

	return 0;
}

/*If it is a single credentials*/
int proxy_auth_credentials_parser (batch_context*const bctx, char*const value)
{
#if 0
  size_t string_len = 0;

  if (! (string_len = strlen (value)))
    {
      fprintf(stderr, "%s - warning: empty PROXY_AUTH_CREDENTIALS\n", 
              __func__);
      return 0;
    }

  string_len++;
  
  if (! (bctx->url_ctx_array[bctx->url_index].proxy_auth_credentials = 
       (char *) calloc (string_len, sizeof (char))))
    {
      fprintf(stderr, 
                  "%s error: failed to allocate memory for PROXY_AUTH_CREDENTIALS" 
                  "with errno %d.\n",  __func__, errno);
      return -1;
    }

  const char separator = ':';
  if (!strchr (value, separator))
    {
      fprintf(stderr, 
                  "%s error: separator (%c) of username and password to be "
              "present in the credentials string \"%s\"\n", 
              __func__, separator, value);
      return -1;
    }

  strncpy (bctx->url_ctx_array[bctx->url_index].proxy_auth_credentials, 
           value, 
           string_len -1);

  return 0;
#endif
  size_t string_len = 0;
  const char separator = ':';


  if ((string_len = strlen (value)) < CLIENT_CRED_LENGTH_MIN)
    {
      fprintf(stderr, "%s - warning: Error PROXY_AUTH_CREDENTIALS: too short. bctx=%p\n", 
              __func__, bctx);
      return 0;
    }
  
  if (!strchr (value, separator))
    {
      fprintf(stderr, 
                  "%s error: separator (%c) of username and password to be "
              "present in the credentials string \"%s\"\n", 
              __func__, separator, value);
      return -1;
    }

    alloc_and_store_credentials(value, 0);
	
    return 0;
}

int access_debug_flag_parser(batch_context*const bctx, char*const value)
{
    long boo = atol (value);

	if (!bctx)
	{
		return -1;
	}

	if (boo < 0 || boo > 1)
	{
		fprintf(stderr, "%s error: boolean input 0 or 1 is expected\n", __func__);
		return -1;
	}
    access_debug_flag = boo;
    return 0;

}


void write_log_for_multi_creds(char* str)
{
	char file_out[]="access_log";
	FILE* fp=NULL;
	struct stat f_stat;

	if (!access_debug_flag)
	{
		return;
	}

	fp=fopen(file_out, "a");
	fputs(str, fp);
	fclose(fp);

	if (stat(file_out, &f_stat) == -1)
	{
		return;
	}

	if (f_stat.st_size>2*1024*1024)
	{
		fp=fopen(file_out, "w");
		fclose(fp);
	}

	return;
}



int bind_credentials_with_client(client_context*const cctx)
{
	unsigned int index_based_on_ip=0;

	if (proxy_auth_method == CURLAUTH_NONE)
	{
		return 0;
	}

    if (proxy_credentials_type)
	{
		index_based_on_ip=(unsigned int)ntohl(inet_addr(cctx->bctx->ip_addr_array [cctx->client_index]))%client_creds_num;
	}
	
	cctx->client_credentials=(char *)malloc(CLIENT_CRED_LENGTH_MAX);
	if (!cctx->client_credentials)
	{
		return -1;
	}

	strncpy(cctx->client_credentials, client_creds_array[index_based_on_ip], CLIENT_CRED_LENGTH_MAX);
	cctx->client_credentials_index=index_based_on_ip;

	return 0;
}


void setup_proxy_credentials_for_client(client_context*const cctx)
{
	CURL* handle = cctx->handle;

	if (proxy_auth_method == CURLAUTH_NONE)
	{
		return;
	}

	if (is_transparent_proxy)
	{
		curl_easy_setopt (handle, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt (handle, CURLOPT_UNRESTRICTED_AUTH, 1);

		curl_easy_setopt(handle, CURLOPT_HTTPAUTH, proxy_auth_method);
		curl_easy_setopt(handle, CURLOPT_USERPWD, cctx->client_credentials);
	}
	else
	{
		curl_easy_setopt(handle, CURLOPT_PROXYAUTH, proxy_auth_method);	
		curl_easy_setopt(handle, CURLOPT_PROXYUSERPWD, cctx->client_credentials);
	}

	return;
}






