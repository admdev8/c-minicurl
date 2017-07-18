typedef struct addrinfo addrinfo;

addrinfo* socks_addrinfo_init(char*, int, addrinfo*, addrinfo);
int socks_init(addrinfo *servinfo);