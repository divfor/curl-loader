
extern int client_creds_num;
extern authentication_method  proxy_auth_method;

extern void write_log_for_multi_creds(char * str);
extern int bind_credentials_with_client(client_context*const cctx);
extern void setup_proxy_credentials_for_client(client_context*const cctx);
extern int load_creds_from_file(char* filename);


extern int proxy_auth_credentials_parser (batch_context*const bctx, char*const value);

extern int credentials_type_parser(batch_context*const bctx, char*const value);
extern int credentials_record_file_parser(batch_context*const bctx, char * const value);
extern int access_debug_flag_parser(batch_context*const bctx, char * const value);

extern int free_client_credentials();


