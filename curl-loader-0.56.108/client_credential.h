
extern int client_creds_num;
extern authentication_method  proxy_auth_method;

extern void write_log_for_multi_creds(char * str);
extern void cc_set_credentials_for_client(client_context*const cctx);
extern int cc_free_client_credentials();

extern int proxy_auth_credentials_parser (batch_context*const bctx, char*const value);
extern int credentials_type_parser(batch_context*const bctx, char*const value);
extern int credentials_record_file_parser(batch_context*const bctx, char * const value);
extern int access_debug_flag_parser(batch_context*const bctx, char * const value);



