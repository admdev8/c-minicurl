// Standard Things
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Minicurl Utils
#include "../inc/utils.h"
#include "../inc/socks.h"
#include "../inc/http.h"


// Formulate a valid GET request and call connect(<socket>)
//////////////////////////////////////////////////////////////////
void http_get_request(char* hostname, char* hostpath, addrinfo* servinfo, int sock_fd){

  char msg[255];
  strcpy(msg, "GET ");
  strcat(msg, hostpath);
  strcat(msg, " HTTP/1.1\r\nHost: ");
  strcat(msg, hostname);
  strcat(msg, "\r\n\r\n");
  int len = strlen(msg);
   
  printf("-----------------------------------\n");
  printf("%s", msg); 
  printf("-----------------------------------\n");

  connect(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen);
  send(sock_fd, (void*)msg, len, 0);
}

// Recieve packet data from TCP socket and print to stdout
/////////////////////////////////////////////////////////////////
void http_write_to_stdout(int sock_fd){

  int SIZE = 8192;
  void *buf = (void*)malloc(sizeof(char)*SIZE*2);   // Can store two packets
  void *tmp = (void*)malloc(sizeof(char)*SIZE*1);   // Can store one packet 

  int recv_len = 0;
  int packet_len = 0;
  packet_len = recv(sock_fd, (void*)buf, SIZE, 0);  // Revieve initial packet
  int   data_len  = get_file_size(buf);             // Get filesize from HTTP headers
  char* data_type = get_file_type(buf);             // Get filetype from HTTP headers
  printf("\b-----------------------------------\n");

  // Request a second packet in case headers spill over into second packet. 
  // This helps us to avoid segfault when we strip headers.    
  recv_len += packet_len;
  if(recv_len < data_len){ // Check to make sure we do not already have everything
    packet_len = recv(sock_fd, (void*)tmp, SIZE, 0);// Revieve second packet
    recv_len += packet_len;
    strcat(buf, tmp);
  }

  // If we have everything, now we can proceed to write to file.
  if ( (strcmp(data_type, "text/html") == 0) ||  
       (strcmp(data_type, "text/plain") == 0)){ // if we have Contetn:Type: text/html  
    printf("%s", strip_headers(buf, SIZE)); // Start writing at <!doctype>
    recv_len -= (strlen(buf) - strlen(strip_headers(buf, SIZE)));

    while (recv_len < data_len || data_len < 0){ // Continue reading packets
      packet_len = recv(sock_fd, (void*)buf, SIZE*2, 0);
      recv_len += packet_len;
      printf("%s", buf);
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
void http_write_to_file(int sock_fd, char* filename){

  int SIZE = 8192;
  void *buf = (void*)malloc(sizeof(char)*SIZE*2);   // Can store two packets
  void *tmp = (void*)malloc(sizeof(char)*SIZE*1);   // Can store one packet 

  int recv_len = 0;
  int packet_len = 0;
  packet_len = recv(sock_fd, (void*)buf, SIZE, 0);  // Revieve initial packet
  int   data_len  = get_file_size(buf);             // Get filesize from HTTP headers
  char* data_type = get_file_type(buf);             // Get filetype from HTTP headers
  printf("\b-----------------------------------\n");

  // Request a second packet in case headers spill over into second packet. 
  // This helps us to avoid segfault when we strip headers.    
  recv_len += packet_len;
  if(recv_len < data_len){ // Check to make sure we do not already have everything
    packet_len = recv(sock_fd, (void*)tmp, SIZE, 0);// Revieve second packet
    recv_len += packet_len;
    strcat(buf, tmp);
  }

  // If we have everything, now we can proceed to write to file.
  if ( (strcmp(data_type, "text/html") == 0) ||  
       (strcmp(data_type, "text/plain") == 0)){ // if we have Contetn:Type: text/html  
    FILE *fp = fopen(filename, "w+"); 
    fprintf(fp, "%s", strip_headers(buf, SIZE)); // Start writing at <!doctype>
    recv_len -= (strlen(buf) - strlen(strip_headers(buf, SIZE)));
    printf("\rWrote %d/%d bytes to %s", recv_len, data_len, filename); 

    while (recv_len < data_len || data_len < 0){ // Continue reading packets
      packet_len = recv(sock_fd, (void*)buf, SIZE*2, 0);
      recv_len += packet_len;
      fprintf(fp, "%s", buf);
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
void http_dns_print(char* hostname, addrinfo* servinfo){

   printf("IP addresses for %s:\n\n", hostname);
   char ipstr[INET6_ADDRSTRLEN];
   struct addrinfo *p;

   for(p = servinfo; p != NULL; p = p->ai_next) {
     void *addr;
     char *ipver;

     // get the pointer to the address itself,
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
