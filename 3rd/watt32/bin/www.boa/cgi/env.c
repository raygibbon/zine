/* The CGI program that displays 
   the server environment vairables 
   Created by Harry H. Cheng, 11/7/1995 
*/

#include<stdio.h>

int main()
{
  printf ("Content-type: text/plain\n\n");
  printf ("CGI environment variable CH test script reports:\n\n");

  printf ("SERVER_SOFTWARE = %s\n",   getenv("SERVER_SOFTWARE"));
  printf ("SERVER_NAME = %s\n",       getenv("SERVER_NAME"));
  printf ("GATEWAY_INTERFACE = %s\n", getenv("GATEWAY_INTERFACE"));
  printf ("SERVER_PROTOCOL = %s\n",   getenv("SERVER_PROTOCOL"));
  printf ("SERVER_PORT = %s\n",       getenv("SERVER_PORT"));
  printf ("REQUEST_METHOD = %s\n",    getenv("REQUEST_METHOD"));
  printf ("HTTP_ACCEPT = %s\n",       getenv("HTTP_ACCEPT"));
  printf ("PATH_INFO = %s\n",         getenv("PATH_INFO"));
  printf ("PATH_TRANSLATED = %s\n",   getenv("PATH_TRANSLATED"));
  printf ("SCRIPT_NAME = %s\n",       getenv("SCRIPT_NAME"));
  printf ("QUERY_STRING = %s\n",      getenv("QUERY_STRING"));
  printf ("REMOTE_HOST = %s\n",       getenv("REMOTE_HOST"));
  printf ("REMOTE_ADDR = %s\n",       getenv("REMOTE_ADDR"));
  printf ("REMOTE_USER = %s\n",       getenv("REMOTE_USER"));
  printf ("AUTH_TYPE = %s\n",         getenv("AUTH_TYPE"));
  printf ("CONTENT_TYPE = %s\n",      getenv("CONTENT_TYPE"));
  printf ("CONTENT_LENGTH = %s\n",    getenv("CONTENT_LENGTH"));
  return (0);
}
