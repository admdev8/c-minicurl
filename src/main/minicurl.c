/* minicurl: A minimalistic version of curl for demonstration */ 
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
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

// Local Libraries
#include "utils.h"
#include "socks.h"
#include "http.h"
#include "https.h"

// Mode selectors
#define NONE 0
#define HTTP 80
#define HTTPS 443

int main(int argc, char **argv)
{


   // An ugly section to parse user input 
   ///////////////////////////////////////////////////////////////////
   char url[256];
   char filename[128];
   int file_mode;   
   int valid = 0;
   int force_ssl = 0;

   fprintf(stderr,"-----------------------------------\n");
   fprintf(stderr,"-            minicurl             -\n");
   fprintf(stderr,"-----------------------------------\n");


   if (argc == 1) {
      fprintf(stderr,"-              usage              -\n");
      fprintf(stderr,"-----------------------------------\n");
      fprintf(stderr,"minicurl <url>\n");
      fprintf(stderr, "\t --> Download <url> to ./index.html\n\n");

      fprintf(stderr,"minicurl <url> -f <filename>\n");
      fprintf(stderr, "\t --> Download <url> to <filename>\n\n");

      fprintf(stderr,"minicurl <url> -d \n");
      fprintf(stderr, "\t --> Download <url> to stdout\n\n");

      fprintf(stderr,"minicurl <url> -ssl\n");
      fprintf(stderr, "\t --> Download <url> to ./index.html\n");
      fprintf(stderr, "\t --> Force HTTPS/SSL\n\n");      
      return 1;
   }
   else if (argc == 2){
      strcpy(url, argv[1]);
      strcpy(filename, "./index.html");
      file_mode = 1; 
      valid =1;
   }
   else{
      int idx;
      for (idx = 1; idx < argc;  idx++) {
   
         if (strcmp(argv[idx], "-ssl") == 0){
            force_ssl = 1;  
            strcpy(url, argv[1]);
            strcpy(filename, "./index.html");
            file_mode = 1; 
            valid =1;
         }  
         if (strcmp(argv[idx], "-f") == 0) {
            strcpy(url, argv[1]);
            strcpy(filename, argv[idx+1]);
            file_mode = 1;
            valid = 1;
            break;
            
         }
         if (strcmp(argv[idx], "-d") == 0){
            strcpy(url, argv[1]);
            strcpy(filename, "./index.html");   
            file_mode = 0;
            valid=1;
            break;
         }
      } 
   }
   if (!valid){ // Case of invalid flags. Note we do not check for valid url
      fprintf(stderr,"-              usage              -\n");
      fprintf(stderr,"-----------------------------------\n");

      fprintf(stderr,"minicurl <url>\n");
      fprintf(stderr, "\t --> Download <url> to ./index.html\n\n");

      fprintf(stderr,"minicurl <url> -f <filename>\n");
      fprintf(stderr, "\t --> Download <url> to <filename>\n\n");

      fprintf(stderr,"minicurl <url> -d \n");
      fprintf(stderr, "\t --> Download <url> to stdout\n\n");

      fprintf(stderr,"minicurl <url> -ssl\n");
      fprintf(stderr, "\t --> Download <url> to ./index.html\n");
      fprintf(stderr, "\t --> Force HTTPS/SSL\n\n");
      return 1;          
   }
   fprintf(stdout, "\n");
   // End User Input 
   ///////////////////////////////////////////////////////////////////
   // Here we would add URL validation ...
   char* hostname = get_url_base(url);
   char* hostpath = get_url_path(url);
   char* hostmode = get_url_mode(url);
 
   // Set up our configuration struct for getaddrinfo. This specifies
   // some basics about how we will achieve our connection. 
   ///////////////////////////////////////////////////////////////////
   struct addrinfo cfg;           // We will fill this and getaddrinfo  
   memset(&cfg, 0, sizeof cfg);   // Make sure the struct is empty
   cfg.ai_family   = AF_UNSPEC;   // don't care IPv4 or IPv6
   cfg.ai_socktype = SOCK_STREAM; // TCP stream sockets
   cfg.ai_flags    = AI_PASSIVE;  // Fill in my IP for me

   // Declare our struct for storing the results of getaddrinfo ... 
   ///////////////////////////////////////////////////////////////////
   struct addrinfo *servinfo;     // Points to results
   int sock_fd;                   // Socket file descriptor    


   // Set up socket mode
   ///////////////////////////////////////////////////////////////////
   int mode;
   if (!strcmp(hostmode, "none")){mode = HTTP;}
   if (!strcmp(hostmode, "http")){mode = HTTP;}
   if (!strcmp(hostmode, "https")){mode = HTTPS;}
   if (force_ssl){mode = HTTPS;}   
   
   switch( mode ){
      // HTTPS/SSL Code Macroblocks
      ////////////////////////////////////////////////////////////////////////////////////
      case HTTPS: 
         servinfo = socks_addrinfo_init(hostname, HTTPS, servinfo, cfg); // getaddrinfo (DNS)
         sock_fd = socks_init(servinfo);                             // Initialize socket
         https_dns_print(hostname, servinfo);                        // Debug

         SSL_CTX* ssl_ctx = https_init_ssl();                        // Initialize SSL
         SSL* sock_ssl = https_get_request(hostname, hostpath,       // SSL_write GET request
                                           servinfo, sock_fd, ssl_ctx);           

         (file_mode) ? https_write_to_file(sock_ssl, filename)       // SSL_read data
                     : https_write_to_stdout(sock_ssl); 

         SSL_free(sock_ssl);                                         // Free sock_ssl
         freeaddrinfo(servinfo);                                     // Free servinfo      
         return 0;
   
      // HTTP Code Macroblocks
      ////////////////////////////////////////////////////////////////////////////////////
      case HTTP: 
         servinfo = socks_addrinfo_init(hostname, HTTP, servinfo, cfg); // getaddrinfo (DNS)
         sock_fd = socks_init(servinfo);                            // Initialize socket

         http_dns_print(hostname, servinfo);                        // Debug
         http_get_request(hostname, hostpath, servinfo, sock_fd);   // SEND GET request   
         (file_mode) ? http_write_to_file(sock_fd, filename)        // RECV data
                     : http_write_to_stdout(sock_fd); 

         freeaddrinfo(servinfo);                                    // Free servinfo
         return 0;
   }   
}