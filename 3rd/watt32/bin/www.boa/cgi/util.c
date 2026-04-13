/***********************************************************
* File: util.c
* Purpose: The utility functions for handling FORM 
*          in both POST and GET methods on the WWW
*
* Note: (1) This is created just for C compilers that 
*           do not support nested functions.
* History: created by Harry H. Cheng, 11/24/1995
*          Last time modified, 10/25/1996
*          hhcheng@ucdavis.edu
**********************************************************/
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

/***********************************************************
* Function File: getnamevalue.c
* Purpose: The function obtains the name and value pairs 
*          name=value&name=value submitted through FORM on the WWW      
*
* Note: (1) Function can handle both POST and GET request methods
*       (2) The memory for name and value is located here, it shall be
*           deleted by delnamevalue()
*       (3) The name[i]/value[i] are passed as arguments.
*           For a given name[i], if there is no value passed, 
*           the corresponding value[i] is NULL.
*       (4) return number of name/value pairs if successful;
*                  -1                         if not invoked through CGI
*                  -2                         not enough memory
*
* History: created by Harry H. Cheng, 11/24/1995
**********************************************************/
int getnamevalue(char ***name, char ***value)
{
  int cl;                    /* content length */
  int i, l1, l2, num=1;
  char *qs, *clientinput, *token;
  char **nam, **val;

  if(getenv("REQUEST_METHOD")==NULL) { /* not CGI */
    printf("Warning: getnamevalue() is invoked not through WWW-CGI\n");
    return -1;
  }
  else if(!strcmp("POST",getenv("REQUEST_METHOD"))) { /* POST */
    cl = atoi(getenv("CONTENT_LENGTH"));
    if(cl == 0)
      return 0; /* send nothing */
    clientinput = (char*) malloc(sizeof(char)*(cl+1));
    if(clientinput == NULL)
      return -2;
    fgets(clientinput, cl+1, stdin); /* get the client input */ 
  }
  else { /* GET */
    qs = getenv("QUERY_STRING");
    if(qs == NULL)
      return 0; /* send nothing */
    else {
      cl = strlen(qs); 
      clientinput = (char*) malloc(sizeof(char)*(cl+1));
      if(clientinput == NULL)
        return -2;
      strcpy(clientinput,qs);
    }
  }

  token = strchr(clientinput, '&'); /* & is the name/value pair separator */
  while(token!=NULL) {        /* obtain the total number of pairs */
    num++;
    token++;
    token = strchr(token, '&');
  }

  nam = (char **)malloc(sizeof(char*)*num);
  val = (char **)malloc(sizeof(char*)*num);
  if(nam == NULL || val == NULL)
    return -2;
  for (i=0, token=strtok(clientinput, "&"); token!=NULL; i++) {
    l1 = strlen(token);
    l2 = strcspn(token,"=");
    nam[i] = (char *)malloc(sizeof(char) * (l2+1));
    if(nam[i] == NULL)
      return -2;
    strncpy(nam[i], token,l2); 
    nam[i][l2] = '\0';
    unescapechar(nam[i]);
    if(l1 != l2+1 ) { /* name=value& */
      val[i] = (char *)malloc(sizeof(char) * (l1-l2));
      if(val[i] == NULL)
        return -2;
      strcpy(val[i], token+l2+1);
      unescapechar(val[i]);
    }
    else /* special case of name=& */
      val[i] = NULL;
    token=strtok(NULL, "&");
  }
  free(clientinput);
  *name  = nam;
  *value = val;
  return num;   /* return the number of entries */
}

/***********************************************************
* Function File: delnamevalue.c
* Purpose: The function delete the memory for 
*          name/value submitted through FORM on the WWW      
*
* Note: (1)The memory for name and value is allocated in getnamevalue()
*
* History: created by Harry H. Cheng, 11/24/1995
**********************************************************/
void delnamevalue(char **name, char **value, int num) {
  int i;
  if(num==0)
    return;
  for (i = 0; i<num; i++) {
    if(name[i])
      free(name[i]);
    if(value[i])
      free(value[i]);
  }
  free(name);
  free(value);
}

/***********************************************************
* Function File: unescapechar.c
* Purpose: The function un-escape characters escaped by the WWW 
*
* Note: (1) change '+' to ' '
*       (2) change "%xx" as a single character
*
* History: created by Harry H. Cheng, 11/24/1995
**********************************************************/
  int unescapechar(char *url) {
    int x,y;
    char hex[3]; /* contains xx of %xx */

    for(x=0,y=0;url[y];++x,++y) {
        if(url[y] == '+')
          url[x] = ' ';       /* change '+' to ' ' */
        else if(url[y] == '%') { /* change xx as char */
          hex[0] = url[y+1]; hex[1] = url[y+2]; hex[2] = '\0';
          url[x] = strtol(hex, NULL, 16);
          y+=2;              /* three chars %xx as one char */
        }
        else
          url[x] = url[y];
    }
    url[x] = '\0';
  }

