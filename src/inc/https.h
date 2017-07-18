typedef struct addrinfo addrinfo;

SSL_CTX* https_init_ssl(void);
SSL* https_get_request(char*, char*, addrinfo*, int, SSL_CTX*);
void https_write_to_stdout(SSL*);
void https_write_to_file(SSL*, char*);
void https_dns_print(char*, addrinfo*);
int https_portscan(char*, int); 