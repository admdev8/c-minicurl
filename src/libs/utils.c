// Standard Things
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Regex
#include <regex.h>

// Some poorly coded but working regex parsers for URL handling

// Returns http://<<<www.domain.com>>>/this/ext.html
char* get_url_base(char* src){ 

   regex_t regex;
   int reti;
   int len;
   reti = regcomp(&regex, "([^\\/]+\\.)+[^\\/]+", REG_EXTENDED);

   regmatch_t pmatch[10];
   reti = regexec(&regex, src, 10, pmatch, REG_NOTBOL);
   if(reti == REG_NOMATCH)
   {
      printf("Invalid URL\n");
      exit(0);
   }
   
   int i;
   char result[256];
   len = pmatch[0].rm_eo - pmatch[0].rm_so;
   memcpy(result, src + pmatch[0].rm_so, len);
   result[len] = 0;
   
   char* url_base = (char*) malloc(sizeof(char)*256);
   strcpy(url_base, result);
   return url_base;
}

// Returns http://www.domain.com<<</this/ext.html>>>
char* get_url_path(char* src){ 

   regex_t regex;
   int reti;
   int len;

   reti = regcomp(&regex, "[^:\\/]((\\/[^\\/]+)+)", REG_EXTENDED);

   regmatch_t pmatch[10];
   reti = regexec(&regex, src, 10, pmatch, REG_NOTBOL);
   if(reti == REG_NOMATCH){ 
     char* url_path = (char*) malloc(sizeof(char)*256);
     strcpy(url_path, "/");
     return url_path;
   }
   else {
     int i;
     char result[256];
     len = pmatch[1].rm_eo - pmatch[1].rm_so;
     memcpy(result, src + pmatch[1].rm_so, len);
     result[len] = 0;
     char* url_path = (char*) malloc(sizeof(char)*256);
     strcpy(url_path, result);
     return url_path;
   }   
}

// Returns <<<http>>>://www.domain.com/this/ext.html 
char* get_url_mode(char* src){ 

   regex_t regex;
   int reti;
   int len;
   reti = regcomp(&regex, "([a-z]+):\\/\\/", REG_EXTENDED);

   regmatch_t pmatch[100];
   reti = regexec(&regex, src, 100, pmatch,  REG_NOTBOL);
   if(reti == REG_NOMATCH)
   {
     char* url_path = (char*) malloc(sizeof(char)*256);
     strcpy(url_path, "none");
     return url_path;
   }
   int i;
   char result[256];
   for (i = 0; pmatch[i].rm_so != -1; i++)
   {
     len = pmatch[i].rm_eo - pmatch[i].rm_so;
     memcpy(result, src + pmatch[i].rm_so, len);
     result[len] = 0;
   }

   char* url_mode = (char*) malloc(sizeof(char)*256);
   strcpy(url_mode, result);
   return url_mode;
}

//////////////////////////////////////////////////////////////////////////////
int get_file_size(char* src){

   regex_t regex;
   int reti;
   int len;

   // Does not work with chunked transfer encodings
   //reti = regcomp(&regex, "\\(Content-Length:\\s\\)\\([0-9]\\{1,9\\}\\)", 0);

   reti = regcomp(&regex, "Content-Length:\\s*([0-9]{1,9})", REG_EXTENDED);

   if (reti) {
     fprintf(stderr, "Could not compile regex\n");
     exit(1);
   }

   regmatch_t pmatch[10];
   reti = regexec(&regex, src, 10, pmatch, REG_NOTBOL);
   if(reti == REG_NOMATCH)
   {
      printf("Content-Length Missing -- Passing\n");
      return -1;
   }
   
   char result[256];
   len = pmatch[0].rm_eo - pmatch[0].rm_so;
   memcpy(result, src + pmatch[0].rm_so, len);
   result[len] = 0;
   printf("%s\n", result);
   
   len = pmatch[1].rm_eo - pmatch[1].rm_so;
   memcpy(result, src + pmatch[1].rm_so, len);
   result[len] = 0;
   return atoi(result); 
}



// Does not work with chunked transfer encodings
char* get_file_type(char* src){

   regex_t regex;
   int reti;
   int len;

   // Does not work with chunked transfer encodings
   reti = regcomp(&regex, "Content-Type:\\s*(\\S+[\\/][^;\\s\n\r]*)", REG_EXTENDED);
   if (reti) {
     fprintf(stderr, "Could not compile regex\n");
     exit(1);
   }

   regmatch_t pmatch[10];
   reti = regexec(&regex, src, 10, pmatch, REG_NOTBOL);
   char* result = (char*)malloc(256*sizeof(char));

   //printf(src);
   if(reti == REG_NOMATCH)
   {
      printf("Content-Type Missing -- Passing\n");
      strcpy(result, "type/none");
      return result;
   }
   
   len = pmatch[0].rm_eo - pmatch[0].rm_so;
   memcpy(result, src + pmatch[0].rm_so, len);
   result[len] = 0;
   printf("%s\n", result);
   
   len = pmatch[1].rm_eo - pmatch[1].rm_so;
   memcpy(result, src + pmatch[1].rm_so, len);
   result[len] = 0;
   return result; 
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
  return strlen(src) - strlen(tmp);   
}
