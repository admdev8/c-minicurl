typedef struct addrinfo addrinfo;

void http_get_request(char*, char*, addrinfo*, int);
void http_write_to_stdout(int);
void http_write_to_file(int, char*);
void http_dns_print(char*, addrinfo*);