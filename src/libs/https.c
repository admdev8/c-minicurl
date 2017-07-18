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

// Minicurl Utils
#include "../inc/utils.h"
#include "../inc/https.h"
#include "../inc/socks.h"


// Initialize SSL library and set ssl_ctx in case we are dealing
// with https connections ( port 443 )
///////////////////////////////////////////////////////////////////
SSL_CTX* https_init_ssl(){
   OpenSSL_add_all_algorithms();
   SSL_load_error_strings();

   if(SSL_library_init() < 0){
      printf("Could not initialize the OpenSSL library !\n");
   }
   
   SSL_CTX *ssl_ctx = SSL_CTX_new(SSLv23_client_method());
   SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2);
   return ssl_ctx;
}

// Formulate a valid GET request and call connect(<socket>)
// Here we implement the SSL layer on our socket.
//////////////////////////////////////////////////////////////////
SSL* https_get_request(char* hostname, char* hostpath, addrinfo* servinfo, int sock_fd, SSL_CTX* ssl_ctx){
   
   // Create standard TCP socket
   connect(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen);

   // create an SSL connection and attach it to the socket
   SSL* sock_ssl = SSL_new(ssl_ctx);
   SSL_set_fd(sock_ssl, sock_fd);
   
   // perform the SSL/TLS handshake with the server - when on the
   // server side, this would use SSL_accept()
   int err = SSL_connect(sock_ssl);

   // SSL Error Checking (for debug)
   if (err != 1) {
    printf("Error: Could not build a SSL session to: %s.\n", hostname);
    int errcode = SSL_get_error(sock_ssl, err);
    switch(errcode){
        case SSL_ERROR_NONE:
	  break; // Cannot happen if err <=0
        case SSL_ERROR_ZERO_RETURN:
	  fprintf(stderr,"SSL connect returned 0.");break;
        case SSL_ERROR_WANT_READ:
	  fprintf(stderr,"SSL connect: Read Error.");break;
        case SSL_ERROR_WANT_WRITE:
	  fprintf(stderr,"SSL connect: Write Error.");break;
        case SSL_ERROR_WANT_CONNECT:
	  fprintf(stderr,"SSL connect: Error connect."); break;
        case SSL_ERROR_WANT_ACCEPT:
	  fprintf(stderr,"SSL connect: Error accept."); break;
        case SSL_ERROR_WANT_X509_LOOKUP:
	  fprintf(stderr,"SSL connect error: X509 lookup."); break;
        case SSL_ERROR_SYSCALL:
	  fprintf(stderr,"SSL connect: Error in system call."); break;
        case SSL_ERROR_SSL:
	  fprintf(stderr,"SSL connect: Protocol Error."); break;
        default:
	  fprintf(stderr,"Failed SSL connect.");
     }
  }
  else {
    printf("SSL Certificate Data: %s\n", hostname);
    printf("-----------------------------------\n  ");

    X509 *cert = NULL;
    cert = SSL_get_peer_certificate(sock_ssl);
  
    X509_NAME *certname = NULL;
    certname = X509_NAME_new();
    certname = X509_get_subject_name(cert);

    BIO *outbio = NULL;
    outbio  = BIO_new_fp(stdout, BIO_NOCLOSE);
    X509_NAME_print_ex(outbio, certname, 0, 0);

  } 
  // Construct our http GET request
  char msg[255];
  strcpy(msg, "GET ");
  strcat(msg, hostpath);
  strcat(msg, " HTTP/1.1\r\nHost: ");
  strcat(msg, hostname);
  strcat(msg, "\r\n\r\n");
  int len = strlen(msg);
   
  printf("\n\nHTTP Request Data:\n");
  printf("-----------------------------------\n");
  printf("%s", msg); 
  // Write our get request on socket_ssl
  SSL_write(sock_ssl, (void*)msg, len);
  return sock_ssl;
}

// Recieve packet data from TCP socket and print to stdout
/////////////////////////////////////////////////////////////////
void https_write_to_stdout(SSL* sock_ssl){

  int SIZE = 8192;
  void *buf = (void*)malloc(sizeof(char)*SIZE*2);   // Can store two packets
  void *tmp = (void*)malloc(sizeof(char)*SIZE*1);   // Can store one packet 

  int recv_len = 0;
  int packet_len = 0;
  packet_len = SSL_read(sock_ssl, (void*)buf, SIZE);// Revieve initial packet
  int   data_len  = get_file_size(buf);             // Get filesize from HTTP headers
  char* data_type = get_file_type(buf);             // Get filetype from HTTP headers
  printf("\b-----------------------------------\n");

  // Request a second packet in case headers spill over into second packet. 
  // This helps us to avoid segfault when we strip headers.    
  recv_len += packet_len;
  if(recv_len < data_len){ // Check to make sure we do not already have everything
    packet_len = SSL_read(sock_ssl, (void*)tmp, SIZE);// Revieve second packet
    recv_len += packet_len;
    strcat(buf, tmp);
  }

  // If we have everything, now we can proceed to write to file.
  if ( (strcmp(data_type, "text/html") == 0) ||  
       (strcmp(data_type, "text/plain") == 0)){ // if we have Contetn:Type: text/html 
    printf("%s", (char*)strip_headers(buf, SIZE*2)); // Start writing at <!doctype>
    recv_len -= (strlen(buf) - strlen(strip_headers(buf, SIZE)));

    while (recv_len < data_len || data_len < 0){ // Continue reading packets
      packet_len = SSL_read(sock_ssl, (void*)buf, SIZE*2);
      recv_len += packet_len;
      printf("%s", (char*)buf);
      if(packet_len == 0){break;}
    }
  }
  // If we want to extend to include more data types we can do so by adding
  // more control flow blocks. 
  else {
    printf("Content-Type not text/html: exiting");
  }
  printf("\n\n");
  free(buf);              // free buffer
  free(tmp);              // free tmp
  return;
}

// Recieve packet data from TCP socket and print to file
/////////////////////////////////////////////////////////////////
void https_write_to_file(SSL* sock_ssl, char* filename){

  int SIZE = 8192;
  void *buf = (void*)malloc(sizeof(char)*SIZE*2);   // Can store two "packets"
  void *tmp = (void*)malloc(sizeof(char)*SIZE*1);   // Can store one "packet" 

  int recv_len = 0;
  int packet_len = 0;
  packet_len = SSL_read(sock_ssl, (void*)buf, SIZE);// Revieve initial packet
  int   data_len  = get_file_size(buf);             // Get filesize from HTTP headers
  char* data_type = get_file_type(buf);             // Get filetype from HTTP headers
  printf("\b-----------------------------------\n");

  // Request a second packet in case headers spill over into second packet. 
  // This helps us to avoid segfault when we strip headers.    
  recv_len += packet_len;
  if(recv_len < data_len){ // Check to make sure we do not already have everything
    packet_len = SSL_read(sock_ssl, (void*)tmp, SIZE);// Revieve second packet
    recv_len += packet_len;
    strcat(buf, tmp);
  }

  // If we have everything, now we can proceed to write to file.
  if ( (strcmp(data_type, "text/html") == 0) ||  
       (strcmp(data_type, "text/plain") == 0)){ // if we have Contetn:Type: text/html 
    FILE *fp = fopen(filename, "w+"); 
    fprintf(fp, "%s", (char*)strip_headers(buf, SIZE)); // Start writing at <!doctype>
    recv_len -= (strlen(buf) - strlen(strip_headers(buf, SIZE)));

    printf("\rWrote %d/%d bytes to %s", recv_len, data_len, filename); 

    while (recv_len < data_len || data_len < 0){ // Continue reading packets
      packet_len = SSL_read(sock_ssl, (void*)buf, SIZE*2);
      recv_len += packet_len;
      fprintf(fp, "%s", (char*)buf);
      printf("\rWrote %d/%d bytes to %s", recv_len, data_len, filename); 
      if(packet_len == 0){break;}
    }
    fclose(fp);             // close filepointer
  }
  // If we want to extend to include more data types we can do so by adding
  // more control flow blocks. 
  else {
    printf("Content-Type not text/html: exiting");
  }
  printf("\n\n");
  free(buf);              // free buffer
  free(tmp);              // free tmp
  return;
}

// Print out useful information from getaddrinfo that is stored in
// servinfo. Note that servinfo = a linked list of addrinfo structs.
///////////////////////////////////////////////////////////////////
void https_dns_print(char* hostname, addrinfo* servinfo){

  printf("IP addresses for %s:\n", hostname);
  printf("-----------------------------------\n");
  char ipstr[INET6_ADDRSTRLEN];
  struct addrinfo *p;

  for(p = servinfo; p != NULL; p = p->ai_next) {
    void *addr;
    char *ipver;

    // Get the pointer to the address itself,
    // different fields in IPv4 and IPv6:
    if (p->ai_family == AF_INET) { // IPv4
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4->sin_addr);
      ipver = "IPv4";
    } else { // IPv6
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      addr = &(ipv6->sin6_addr);
      ipver = "IPv6";
    }

    // convert the IP to a string and print it:
    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    printf("  %s: %s\n\n", ipver, ipstr);
  }
}


int https_portscan(char* hostname, int port){

  struct hostent *host; 
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  if( (host = gethostbyname(hostname)) != 0){
    strncpy((char*)&sa.sin_addr , (char*)host->h_addr , sizeof sa.sin_addr);
  }
  sa.sin_port = htons(port);

  int sock_fd = socket(AF_INET , SOCK_STREAM , 0);
  fcntl(sock_fd, F_SETFL, O_NONBLOCK);
  int err = connect(sock_fd, (struct sockaddr*)&sa , sizeof sa); 
  

  fd_set fdset;
  struct timeval tv;
  FD_ZERO(&fdset);
  FD_SET(sock_fd, &fdset);
  tv.tv_sec  = 1;             /* 10 second timeout */
  tv.tv_usec = 0;

  if (select(sock_fd + 1, NULL, &fdset, NULL, &tv) == 1)
  {
      int so_error;
      socklen_t len = sizeof so_error;

      getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);

      if (so_error == 0) {
        printf("Scan ::443 Open - Using HTTPS/SSL\n");
        printf("-----------------------------------\n");      
        return 1;
    }
  }
  printf("Scan ::443 Closed - Defaulting HTTP\n");
  printf("-----------------------------------\n");      
  return 0;
}