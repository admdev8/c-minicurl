/* minicurl: A minimalistic version of curl  */ 
/* Copyright (C) 2017  Michael Winters  */
/*
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or  */
/* (at your option) any later version.  */
/*
/* This program is distributed in the hope that it will be useful,  */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of  */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  */
/* GNU General Public License for more details.  */
/*
/* You should have received a copy of the GNU General Public License  */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */

// Standard Things
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Basic Networking Libs
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// Regex
#include <regex.h>

// Open SSL
#include <openssl/ssl.h>

int get_file_size(char* src){

   regex_t regex;
   int reti;
   int len;

   reti = regcomp(&regex, "\\(Content-Length:\\s\\)\\([0-9]\\{1,6\\}\\)", 0);
   if (reti) {
     fprintf(stderr, "Could not compile regex\n");
     exit(1);
   }

   regmatch_t pmatch[10];
   reti = regexec(&regex, src, 10, pmatch, REG_NOTBOL);
   if(reti == REG_NOMATCH)
   {
      printf("NO match\n");
      exit(0);
   }
   
   int i;
   char result[256];
   for (i = 0; pmatch[i].rm_so != -1; i++)
   {
     len = pmatch[i].rm_eo - pmatch[i].rm_so;
     memcpy(result, src + pmatch[i].rm_so, len);
     result[len] = 0;
   }
   return atoi(result); 
}

char* strip_headers(char* src, int size){
  
   char *tmp = (char*)malloc(sizeof(char)*size); 
   tmp  = strstr(src, "\r\n\r\n");  //Strip http headers for output  
   tmp += 4;                        //Move up 4 characters. 
   return tmp;   
}

int get_header_size(char* src, int size){
  
   char *tmp = (char*)malloc(sizeof(char)*size); 
   tmp  = strstr(src, "\r\n\r\n");  //Strip http headers for output  
   tmp += 4;                        //Move up 4 characters.   
   return strlen(src)-strlen(tmp);   
}

int function(int argc, char **argv)
{
   // A user friendly feature  
   ///////////////////////////////////////////////////////////////////
   if (argc == 1) {
        fprintf(stderr,"usage: minicurl <url>\n");
	fprintf(stderr,"usage: minicurl <url> <filename>\n");
        return 1;
   }
   char *hostname = argv[1];      // Set hostname from user input

   
   // Set up our configuration struct for getaddrinfo. This specifies
   // some basics about how we will achieve our connection. 
   ///////////////////////////////////////////////////////////////////
   struct addrinfo cfg;           // We will fill this and getaddrinfo  
   memset(&cfg, 0, sizeof cfg);   // Make sure the struct is empty
   cfg.ai_family   = AF_UNSPEC;   // don't care IPv4 or IPv6
   cfg.ai_socktype = SOCK_STREAM; // TCP stream sockets
   cfg.ai_flags    = AI_PASSIVE;  // Fill in my IP for me

   // Initialize SSL library and set ssl_ctx in case we are dealing
   // with https connections ( port 443 )
   ///////////////////////////////////////////////////////////////////
   //SSL_load_error_strings ();
   //SSL_library_init ();
   //SSL_CTX *ssl_ctx = SSL_CTX_new (SSLv23_client_method ());
   
   // Declare our struct for storing the results of getaddrinfo ... 
   ///////////////////////////////////////////////////////////////////
   struct addrinfo *servinfo;     // will point to the results

  
   // Make call to getaddrinfo. If this magicial function does not
   // work then we will exit the program.
   ///////////////////////////////////////////////////////////////////
   int status = getaddrinfo(hostname, "80", &cfg, &servinfo);
   if (status != 0) {
     fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
     exit(1);
   }

   // perform the SSL/TLS handshake with the server - when on the
   // server side, this would use SSL_accept()
   // int err = SSL_connect(conn);
   //if (err != 1)
   //  abort(); 
   //SSL *conn = SSL_new(ssl_ctx);
   //SSL_set_fd(conn, sock);
   
   
   // Print out useful information from getaddrinfo that is stored in
   // servinfo. Note that servinfo = a linked list of addrinfo structs. 
   ///////////////////////////////////////////////////////////////////
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
 

   // Now we will create a socket so we can formulate our GET request.
   ///////////////////////////////////////////////////////////////////
   char msg[255];
   strcpy(msg, "GET /?st=1 HTTP/1.1\r\nHost: ");
   strcat(msg, hostname);
   strcat(msg, "\r\n\r\n");
     
   int len = strlen(msg);
   int size = 8192;
   char *buf = (char*)malloc(sizeof(char)*size);
   
   // Begin by creating a socket file descriptor (an int). 
   int socket_fd = socket(servinfo->ai_family,
   			  servinfo->ai_socktype,
   			  servinfo->ai_protocol);
   connect(socket_fd, servinfo->ai_addr, servinfo->ai_addrlen);
   send(socket_fd, (void*)msg, len, 0); 


   
   // Recieve data an write file/output to stdout as specified.
   ////////////////////////////////////////////////////////////////////////////////
   //
   // Write to file 
   //
   if (argc > 2){
     FILE *fp = fopen(argv[2], "w+");
     
     int recv_len = 0;
     recv_len = recv(socket_fd, (void*)buf, size, 0);     
     int data_len = get_file_size(buf);           // Get filesize from HTTP headers
     recv_len -= get_header_size(buf, size);      // Get length of HTTP headers 
     fprintf(fp, "%s", strip_headers(buf, size)); // Start writing at <!doctype>
      
     int i;
     while (recv_len < data_len){
       recv_len += recv(socket_fd, (void*)buf, size, 0);     
       printf("\rWrote %d/%d bytes", recv_len, data_len);
       fprintf(fp, "%s", buf);
     }
     printf("\n");
     fclose(fp);             // close filepointer 
     free(buf);              // free buffer
     freeaddrinfo(servinfo); // free the linked list
     return 0;
   }
   //
   // Write to stdout
   //
   else{
     int recv_len = 0;
     recv_len = recv(socket_fd, (void*)buf, size, 0);     
     int data_len = get_file_size(buf);           // Get filesize from HTTP headers
     recv_len -= get_header_size(buf, size);      // Get length of HTTP headers 
     printf("%s", strip_headers(buf, size));      // Start writing at <!doctype>
      
     int i;
     while (recv_len < data_len){
       recv_len += recv(socket_fd, (void*)buf, size, 0);     
       printf("%s", buf);
     }
     printf("\n Wrote %d/%d bytes", recv_len, data_len);
     free(buf);              // free buffer
     freeaddrinfo(servinfo); // free the linked list
     return 0;
   }
}


//recv_len = recv(socket_fd, (void*)buf, size, 0);
//printf("%s", buf);
//printf("%d", recv_len);

//   while recv(socket_fd, buf + idx, buf - idx, 0);

//hints.ai_family   = AF_UNSPEC;   // don't care IPv4 or IPv6
//hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
//hints.ai_flags    = AI_PASSIVE;  // fill in my IP for me
  
//typedef struct{
//  int x;
//  char* y;  
//}args;

// You might not usually need to write to these structures; oftentimes, a call to getaddrinfo()
// to fill out your struct addrinfo for you is all you'll need. You will, however, have to peer
// inside these structs to get the values out, so I'm presenting them here.
//struct addrinfo {
//    int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
//    int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
//    int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
//    int              ai_protocol;  // use 0 for "any"
//    size_t           ai_addrlen;   // size of ai_addr in bytes
//    struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
//    char            *ai_canonname; // full canonical hostname
//    struct addrinfo *ai_next;      // linked list, next node
//};


//struct sockaddr {
//   unsigned short    sa_family;    // address family, AF_xxx, AF_INET (IPv4) or AF_INET6 (IPv6) 
//   char              sa_data[14];  // 14 bytes of protocol address
//}; 

// (IPv4 only--see struct sockaddr_in6 for IPv6).
// This structure makes it easy to reference elements of the socket address. Note that sin_zero
// (which is included to pad the structure to the length of a struct sockaddr) should be set to all
// zeros with the function memset(). Also, notice that sin_family corresponds to sa_family in a struct
// sockaddr and should be set to "AF_INET". Finally, the sin_port must be in Network Byte Order
// (by using htons()!)
//struct sockaddr_in {
//    short int          sin_family;  // Address family, AF_INET
//    unsigned short int sin_port;    // Port number
//   struct in_addr     sin_addr;    // Internet address
//    unsigned char      sin_zero[8]; // Same size as struct sockaddr
//};
// (IPv4 only--see struct in6_addr for IPv6)

// Internet address (a structure for historical reasons)
//struct in_addr {
//    uint32_t s_addr; // that's a 32-bit int (4 bytes)
//};


//struct sockaddr_in sa; // IPv4
//struct sockaddr_in6 sa6; // IPv6
//inet_pton(AF_INET, "10.12.110.57", &(sa.sin_addr)); // IPv4
//inet_pton(AF_INET6, "2001:db8:63b3:1::3490", &(sa6.sin6_addr)); // IPv6







//#include <netinet/in.h>
//#include <sys/socket.h>

//inet_addr , inet_ntoa , ntohs etc
//#include <sys/types.h>

// function getaddrinfo() that does all kinds of good stuff for you, including DNS
// and service name lookups, and fills out the structs you need, besides!
//
//
// int getaddrinfo(const char *node,     // e.g. "www.example.com" or IP
//                 const char *service,  // e.g. "http" or port number
//                 const struct addrinfo *hints,
//                 struct addrinfo **res);



