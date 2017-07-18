// Standard Things
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

// Basic Networking Libs
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

// Regex
#include <regex.h>

// Open SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

// Minicurl Libs 
#include "../inc/socks.h"

// Make call to getaddrinfo. If this magicial function does not
// work then we will exit the program. Note port 80 for HTTP
///////////////////////////////////////////////////////////////////
addrinfo* socks_addrinfo_init(char* hostname, int port, addrinfo* servinfo, addrinfo cfg){  
  char _port[8];   
  sprintf(_port, "%d", port); //getaddrinfo expects a string
  int status = getaddrinfo(hostname, _port , &cfg, &servinfo);
  if (status != 0) {
     fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
     exit(1);
  }
  return servinfo;
}

// Now we will create a socket so we can formulate our GET request.
///////////////////////////////////////////////////////////////////
int socks_init(addrinfo *servinfo){
   int socket_fd = socket(servinfo->ai_family,
   			  servinfo->ai_socktype,
   			  servinfo->ai_protocol);
   return socket_fd;
}