/* Query the input from the client.
   Report in the form of plain text.
   Created by Harry H. Cheng, 11/7/1995
   Last time modified, 12/28/1995
*/
#include <stdio.h>
#include "www.h"

int main (void)
{
  int         i, num;
  stringArray name;  /* name[i]  is a string of char with a passed name */
  stringArray value; /* value[i] is a string of char with a passed value */

  printf ("Content-type: text/plain\n\n");
  printf ("CGI FORM test script reports:\n");
   
  num = getnamevalue (&name, &value);
  if(num == 0)
  {
    printf ("No input from FORM\n");
    return (0);
  }
  else if (num == -2)
  {
    printf ("No enough memory<p>\n");
    return (0);
  }
  printf ("\nThe following %d name/value pairs are submitted\n\n",num);
  for (i = 0; i < num; i++)
     printf ("%s=%s\n",name[i],value[i]);
  delnamevalue (name, value, num);
  return (0);
}
