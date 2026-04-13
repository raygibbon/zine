/*****************************************/
/* Nasty program to generate numbers. :) */
/*****************************************/
#include <stdio.h>
#include <stdlib.h>
void main(int argc, char *argv[])
{
   register int i;
   int final, princ;

   if (argc == 3)
   {
      princ = atoi(argv[1]);   final = atoi(argv[2]);
      if ( princ <= final )
      {
         for ( i = princ; i <= final; i++ )
            printf("%d ", i);
         return;
      }
   }
#ifdef SPANISH
   printf("Uso:  %s  num.inicio  num.final\n", argv[0]);
#else
   printf("Use:  %s  begin#  end#\n", argv[0]);
#endif
}   
