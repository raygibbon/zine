/* Query the input from the client.
   Report in the form of html format.
   Created by Harry H. Cheng, 11/7/1995
   Last time modified, 12/28/1995
*/
#include <stdio.h>
#include "www.h"

int main()
{
  int         i, num;
  stringArray name;  /* name[i]  is a string of char with a passed name */
  stringArray value; /* value[i] is a string of char with a passed value */

  printf ("Content-type: text/html\n\n");
  printf ("<H1>CGI FORM test script reports:</H1>\n");
   
  num = getnamevalue (&name, &value);
  if (num == 0)
  {
    printf ("No name/value has been submitted<p>\n");
    return (0);
  }
  else if (num == -2)
  {
    printf ("No enough memory<p>\n");
    return (0);
  }

  printf ("The following %d name/value pairs are submitted<p>\n",num);
  printf ("<ul>\n");

  for (i = 0; i < num; i++)
      printf ("<li> <code>%s = %s</code>\n",name[i],value[i]);
  printf ("</ul>\n");

  delnamevalue (name, value, num);
  return (0);
}
