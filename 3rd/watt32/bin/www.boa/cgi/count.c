/* Purpose: count the access number of a web page
** History:
** (1)Originally count.c, by Chris Stephens, stephenc@pcmail.cbil.vcu.edu, (c)1995.
** (2)Code cleaned up and enhanced by Fred Christiansen, fredch@fc.hp.com,
** as odometer.c; supports:
** - 8 digits instead of 7 (and by changing COUNT_DIGITS, up to 10 (number
**      of digits in an unsigned long))
** - acceptance of an argument to create a distinct count file name
** using main(int argv, char **argv) 
** (3) Modified as count.cgi by Harry Cheng for execution in the CH language environment
**     Usage:
**          (a) if count.cgi is invoked by
**                    <img src=http:/cgi-bin/count.cgi>
**              in an html file, by default,
**              COUNT_FILE "/usr/local/ns-home/docs/dotfiles/.count"        
**              will be updated.
**          (b) if count.cgi is invoked byj
**                    <img src=http:/cgi-bin/count.cgi?MyName>
**              in an html file, by default,
**              COUNT_FILE "/usr/local/ns-home/docs/dotfiles/.countMyName"        
**              will be updated.
**          (c) If diff count file is used, change COUNT_FILE.
*               The file must have wr permission such as 666 to Web server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
char *bitmap[] = {
  "0xff","0xff","0xff","0xc3","0x99","0x99","0x99","0x99", /* rows 1-8  of 0 */
  "0x99","0x99","0x99","0x99","0xc3","0xff","0xff","0xff", /* rows 9-16 of 0 */
  "0xff","0xff","0xff","0xcf","0xc7","0xcf","0xcf","0xcf", /* rows 1-8  of 1 */
  "0xcf","0xcf","0xcf","0xcf","0xcf","0xff","0xff","0xff", /* rows 9-16 of 1 */
  "0xff","0xff","0xff","0xc3","0x99","0x9f","0x9f","0xcf", /* rows 1-8  of 2 */
  "0xe7","0xf3","0xf9","0xf9","0x81","0xff","0xff","0xff", /* rows 9-16 of 2 */
  "0xff","0xff","0xff","0xc3","0x99","0x9f","0x9f","0xc7", /* rows 1-8  of 3 */
  "0x9f","0x9f","0x9f","0x99","0xc3","0xff","0xff","0xff", /* rows 9-16 of 3 */
  "0xff","0xff","0xff","0xcf","0xcf","0xc7","0xc7","0xcb", /* rows 1-8  of 4 */
  "0xcb","0xcd","0x81","0xcf","0x87","0xff","0xff","0xff", /* rows 9-16 of 4 */
  "0xff","0xff","0xff","0x81","0xf9","0xf9","0xf9","0xc1", /* rows 1-8  of 5 */
  "0x9f","0x9f","0x9f","0x99","0xc3","0xff","0xff","0xff", /* rows 9-16 of 5 */
  "0xff","0xff","0xff","0xc7","0xf3","0xf9","0xf9","0xc1", /* rows 1-8  of 6 */
  "0x99","0x99","0x99","0x99","0xc3","0xff","0xff","0xff", /* rows 9-16 of 6 */
  "0xff","0xff","0xff","0x81","0x99","0x9f","0x9f","0xcf", /* rows 1-8  of 7 */
  "0xcf","0xe7","0xe7","0xf3","0xf3","0xff","0xff","0xff", /* rows 9-16 of 7 */
  "0xff","0xff","0xff","0xc3","0x99","0x99","0x99","0xc3", /* rows 1-8  of 8 */
  "0x99","0x99","0x99","0x99","0xc3","0xff","0xff","0xff", /* rows 9-16 of 8 */
  "0xff","0xff","0xff","0xc3","0x99","0x99","0x99","0x99", /* rows 1-8  of 9 */
  "0x83","0x9f","0x9f","0xcf","0xe3","0xff","0xff","0xff"  /* rows 9-16 of 9 */
};

#define COUNT_FILE      "/usr/local/ns-home/docs/dotfiles/.count"        
#define UPDATE          "r+"
#define COUNT_DIGITS    8               /* # digits displayed */
#define BUFSIZE         (COUNT_DIGITS+2)  /* 8 digits + newlinechar + null */
#define BM_HT           16              /* bitmap height */
#define BM_WD           8               /* bitmap width */

main()
{
    FILE *fp; 
    char buf[BUFSIZE], dial[COUNT_DIGITS];
    unsigned long cnt;
    int i, l;
    char *qs, *p;
    char *countfile;
    /*string countfile;*/

    /* Establish countfile to count file. */
    qs = getenv("QUERY_STRING");
    if (qs != NULL) {
      countfile = (char*) malloc(strlen(qs)+strlen(COUNT_FILE)+1);
      strcpy(countfile,COUNT_FILE);
      strcat(countfile,qs);
    }
    else {
      countfile = (char*) malloc(strlen(COUNT_FILE)+1);
      strcpy(countfile,COUNT_FILE);
    }

    /* Read current count (and rewind file), bump it, and write it out */
    if(access(countfile,F_OK)) { /* first time */
      fp = fopen(countfile, CREATE);
      if (fp == NULL) {
        printf("Content-type: text/plain\n\n");
        printf("Error: count.cgi failed because file %s cannot be opened\n", countfile);
        exit(-1);
      }  
      else {
        l = 1;
        cnt = 1;
        strcpy(buf, "1");
      }  
    }  
    else {
      fp = fopen(countfile, UPDATE);
      if (fp == NULL) {
        printf("Content-type: text/plain\n\n");
        printf("Error: count.cgi failed because file %s cannot be opened\n", countfile);
        exit(-1);
      }  
      else {
        lockf(fileno(fp), F_LOCK, 0);     /* lock out concurrent writers */
        fgets(buf, BUFSIZE, fp);
        l = strlen(buf)-1;                  /* ignore new line char */
        fseek(fp, 0L, SEEK_SET);
        cnt = strtoul(buf, (char **)NULL, 10);
      }  
    }
    fprintf(fp, "%u\n", ++cnt);
    fclose(fp);
      
    /* copy right-to-left into dial */
    for (i = 0, p = buf; i < l; i++, p++)
	dial[COUNT_DIGITS - l + i] = *p; 
    for (i = 0; i < (COUNT_DIGITS - l); i++)      /* backfill with zeros */
	dial[i] = '0';

    /* Print the XBM definition */
    printf("Content-type: image/x-xbitmap\n\n");
    printf("#define count_width  %d\n", (BM_WD*COUNT_DIGITS));
    printf("#define count_height %d\n", BM_HT);
    printf("static char count_bits[] = {\n");

    for (i = 0; i < BM_HT; i++) {
	for (l = 0; l < COUNT_DIGITS; l++) {
	    printf("%s, ", bitmap[(((dial[l]-'0')*BM_HT)+i)]);
	    if (l == 7)
		printf("\n");
	}
	if (i == 15)
	    printf("};");
    }
    printf("\n");
    exit(0);
}
