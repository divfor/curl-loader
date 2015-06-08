
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
#include <ctype.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <curl/curl.h>
#include <curl/multi.h>

#include <sys/syscall.h>

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

int is_transparent_proxy=1;
static int cc_store_credentials(batch_context*const bctx, char* client_credentials, int index);
static int cc_load_creds_from_file(batch_context*const bctx, char* filename);

/*Is it using a single credential or multi credentials from a file?*/
int credentials_type_parser(batch_context*const bctx, char*const value)
{
	if (!bctx)
	{
		return -1;
	}

	if (!strcmp (value, CREDENTIALS_SINGLE))
	{
		bctx->proxy_credentials_type= 0;
	}
	else if (!strcmp (value, CREDENTIALS_FROM_FILE))
	{
		bctx->proxy_credentials_type = 1;
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
	if (cc_load_creds_from_file (bctx, value) == -1)
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
  
	bctx->client_creds_num=1;
    cc_store_credentials(bctx,value, 0);
	
    return 0;
}

#if 0
static int cl_convert_creds (char *client_credentials, char **principal_name)
{
    char *p1;
    char username[128]={'\0'};
    char domainname[128]={'\0'};
    int domain_sep=92;
    int i=0;

    p1=memchr(domain_and_username, domain_sep, 128);
    if(!p1)
    {
        //printf("Didnot find domain seperator in client_credentials. domain_and_username=%s\n", domain_and_username);
		fprintf (stderr, "Didnot find domain seperator in client_credentials.\n");
        return 1;
    }
    //printf("domain_and_username=%s, p1=%s\n", domain_and_username, p1);

    strncpy(domainname, domain_and_username, (intptr_t)p1-(intptr_t)domain_and_username);
    strncpy(username,(char *)((intptr_t)p1+1), strlen(domain_and_username)-strlen(domainname));
    //printf("domainname=%s,username=%s\n",domainname,username);

    for (i=0; i<strlen(domainname); i++)
    {
        if(islower(domainname[i]))
        {
            domainname[i]=toupper(domainname[i]);
        }
    }

    strncat(principal_name,username, strlen(domain_and_username)-strlen(domainname));
    strncat(principal_name,"@", 1);
    strncat(principal_name,domainname, strlen(domainname));
	principal_name[strlen(domain_and_username)+1]='\0';

    return 0;
}
#endif


static int cc_convert_creds (char *client_credentials, char **principal_name)
{
	char *p1, *p2;
	char username[128]={'\0'};
	char domainname[128]={'\0'};
	char passwd[128]={'\0'};
	int domain_sep=92;    /*    '\'   */
	int passwd_sep=58;    /*    ':'   */
	int i=0;
	int domain_len=0;
	int username_len=0;
	int passwd_len=0;
	char *p_name;

    p1=memchr(client_credentials, domain_sep, CLIENT_CRED_LENGTH_MAX);
    if(!p1)
    {
        //printf("Didnot find domain seperator in client_credentials. domain_and_username=%s\n", domain_and_username);
		fprintf (stderr, "Didnot find domain seperator in client_credentials.\n");
        return -1;
    }
    //printf("domain_and_username=%s, p1=%s\n", domain_and_username, p1);

    p2=memchr(client_credentials, passwd_sep, CLIENT_CRED_LENGTH_MAX);
    if(!p2)
    {
        //printf("Didnot find domain seperator in client_credentials. domain_and_username=%s\n", domain_and_username);
		fprintf (stderr, "Didnot find passwd seperator in client_credentials.\n");
        return -1;
    }
    //printf("domain_and_username=%s, p1=%s\n", domain_and_username, p1);

	domain_len=(intptr_t)p1-(intptr_t)client_credentials;
	username_len=(intptr_t)p2-(intptr_t)p1-1;
	passwd_len=strlen(client_credentials)-domain_len-username_len-1;
	
    strncpy(domainname, client_credentials, domain_len);
    strncpy(username,(char *)((intptr_t)p1+1), username_len);
	strncpy(passwd,(char *)((intptr_t)p2+1), passwd_len);
    //printf("domainname=%s,username=%s, passwd=%s\n",domainname,username, passwd);

    for (i=0; i<domain_len; i++)
    {
        if(islower(domainname[i]))
        {
            domainname[i]=toupper(domainname[i]);
        }
    }

	p_name=(char *)malloc (strlen(client_credentials)+1);
	if (p_name == NULL)
	{
		fprintf (stderr, "%s: malloc error\n", __FUNCTION__);
		return -1;
	}
    strncat(p_name,username, username_len);
    strncat(p_name,"@", 1);
    strncat(p_name,domainname, domain_len);
	strncat(p_name,":", 1);
	strncat(p_name,passwd, passwd_len);
	p_name[strlen(client_credentials)+1]='\0';

	*principal_name=p_name;

	//printf ("client_credentials=%s, p_name=%s\n",client_credentials, p_name );

    return 0;
}

/*Alloc the memory space and store the client_credentials as a global array*/
static int cc_store_credentials(batch_context*const bctx, char* client_credentials, int index)
{
	int i=0;

	/*Alloc the space at the first time(index=0). After that, don't need to do that again*/
	if (index==0)
	{
	    bctx->client_creds_array=(char**)malloc(bctx->client_creds_num*sizeof(char *));
		bctx->client_principle_name_array=(char**)malloc(bctx->client_creds_num*sizeof(char *));
		
		for(i=0; i<bctx->client_creds_num; i++)
		{
		    bctx->client_creds_array[i]=(char*)malloc(CLIENT_CRED_LENGTH_MAX);
			if(!bctx->client_creds_array[i])
			{
			    fprintf(stderr, "%s, malloc error.\n", __FUNCTION__);		  
				return -1;
			}

			memset (bctx->client_creds_array[i], 0, CLIENT_CRED_LENGTH_MAX);
		}
	}

	strncpy(bctx->client_creds_array[index], client_credentials, CLIENT_CRED_LENGTH_MAX);

    /*Deliminated the return char*/
	if ((bctx->client_creds_array[index][strlen(bctx->client_creds_array[index])-1]=='\r')
			||(bctx->client_creds_array[index][strlen(bctx->client_creds_array[index])-1]=='\n'))
	{
		bctx->client_creds_array[index][strlen(bctx->client_creds_array[index])-1]='\0';
	}

	
	cc_convert_creds(bctx->client_creds_array[index], &(bctx->client_principle_name_array[index]));

//printf ("index=%d   cc=%s, pn=%s\n", index, bctx->client_creds_array[index], bctx->client_principle_name_array[index]);


	return 0;
}

int cc_free_client_credentials(batch_context*const bctx)
{
    int i=0;

    if (!bctx->client_creds_array || !bctx->client_principle_name_array)
	{
		return -1;
	}
	
	for(i=0; i<bctx->client_creds_num; i++)
	{
		if(bctx->client_creds_array[i])
		{
			free(bctx->client_creds_array[i]);
			bctx->client_creds_array[i]=NULL;
		}
		if(bctx->client_principle_name_array[i])
		{
			free(bctx->client_principle_name_array[i]);
			bctx->client_principle_name_array[i]=NULL;
		}
	}
    
	free(bctx->client_creds_array);
	free(bctx->client_principle_name_array);

	bctx->client_creds_array=NULL;
	bctx->client_principle_name_array=NULL;
	
    return 0;
}


/*Read the credentials from the config file*/
static int cc_load_creds_from_file(batch_context*const bctx, char* filename)
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

    bctx->client_creds_num=0;
	while(bctx->client_creds_num<CLIENT_CRED_NUM_MAX&&!feof(fp))
	{

		fgets(one_line, CLIENT_CRED_LENGTH_MAX, fp);
		if (strlen(one_line)<CLIENT_CRED_LENGTH_MIN)
		{
			continue;
		}
	
		bctx->client_creds_num++;
	}
    bctx->client_creds_num--;

    fseek(fp,0,SEEK_SET);

	memset(&one_line[0], 0, CLIENT_CRED_LENGTH_MAX);
	while(i<bctx->client_creds_num&&!feof(fp))
	{
		fgets(one_line, CLIENT_CRED_LENGTH_MAX, fp);
		if (strlen(one_line)<CLIENT_CRED_LENGTH_MIN)
		{
			continue;
		}

		cc_store_credentials(bctx, one_line, i);
		i++;
	} 

	fclose(fp);

	return 0;
}


void cc_set_credentials_for_client(client_context*const cctx)
{
	CURL* handle = cctx->handle;
	unsigned int index_based_on_ip=0;
	batch_context* bctx = cctx->bctx;

	if (bctx->proxy_auth_method == CURLAUTH_NONE || bctx->client_creds_num == 0)
	{
		return;
	}

	//index_based_on_ip=(unsigned int)ntohl(inet_addr(bctx->ip_addr_array [cctx->client_index]))%(bctx->client_creds_num);
	index_based_on_ip=(cctx->client_index)%(bctx->client_creds_num);
	//printf ("tid=%ld, client_index=%d, client_creds_num=%d, index_based_on_ip=%d\n", syscall(__NR_gettid), cctx->client_index, bctx->client_creds_num, index_based_on_ip );

	if (is_transparent_proxy)
	{
		curl_easy_setopt (handle, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt (handle, CURLOPT_UNRESTRICTED_AUTH, 1);

		curl_easy_setopt(handle, CURLOPT_HTTPAUTH, bctx->proxy_auth_method);
		if (bctx->proxy_auth_method == CURLAUTH_GSSNEGOTIATE)
		{
			curl_easy_setopt(handle, CURLOPT_USERPWD, bctx->client_principle_name_array[index_based_on_ip]);
		}
		else
		{
			curl_easy_setopt(handle, CURLOPT_USERPWD, bctx->client_creds_array[index_based_on_ip]);
		}
	}
	else
	{
		curl_easy_setopt(handle, CURLOPT_PROXYAUTH, bctx->proxy_auth_method);	
		if (bctx->proxy_auth_method == CURLAUTH_GSSNEGOTIATE)
		{
			//printf ("pn=%s\n", bctx->client_principle_name_array[index_based_on_ip]);
			curl_easy_setopt(handle, CURLOPT_PROXYUSERPWD, bctx->client_principle_name_array[index_based_on_ip]);
		}
		else
		{
			curl_easy_setopt(handle, CURLOPT_PROXYUSERPWD, bctx->client_creds_array[index_based_on_ip]);
		}	
	}

	return;
}






