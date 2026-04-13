/* This file is modified from NCSA's cgi-bin/src/change-passwd.c
   by Harry H. Cheng, 7/18/1996, hhcheng@ucdavis.edu
*/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "www.h"


#define USER_FILE "/usr/local/etc/httpd/conf/.htpasswd"
#define WIZARD    "surobm"
#define LF 10
#define CR 13

#ifndef __CH__
char *crypt(char *pw, char *salt); /* why aren't these prototyped in include */
#endif

char *tn;

/* From local_passwd.c (C) Regents of Univ. of California blah blah
 */
unsigned char itoa64[] =         /* 0 ... 63 => ascii - 64 */
        "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

int to64(char *s, long v, int n)
{
    while (--n >= 0) {
        *s = itoa64[v&0x3f];
        s++;
        v >>= 6;
    }
}

void change_password(char *user, char *pw, FILE *f) {
    char *cpw, salt[3];

    srand((int)time((time_t *)NULL));
    to64(&salt[0],rand(),2);
    cpw = crypt(pw,salt);
    fprintf(f,"%s:%s\n",user,cpw);
}

void putline(FILE *f,char *l) {
    int x;

    for(x=0;l[x];x++) fputc(l[x],f);
    fputc('\n',f);
}

void getword(char *word, char *line, char stopit) {
    int x = 0,y;

    for(x=0;((line[x]) && (line[x] != stopit));x++)
        word[x] = line[x];

    word[x] = '\0';
    if(line[x]) ++x;
    y=0;

    line[y] = line[x];
    while(line[y] == line[x]) {
     y++; x++;
    };
}

int getline(char *s, int n, FILE *f) {
    int i=0;

    while(1) {
        s[i] = (char)fgetc(f);

        if(s[i] == CR)
            s[i] = fgetc(f);

        if((s[i] == 0x4) || (s[i] == LF) || (i == (n-1))) {
            s[i] = '\0';
            return (feof(f) ? 1 : 0);
        }
        ++i;
    }
}

int main(int argc, char *argv[]) {
    int cl, i, num;
    stringArray name;  /* name[i]  is a string of char with a passed name */
    stringArray value; /* value[i] is a string of char with a passed value */
    int found,create;
    char *u,*p1,*p2,*user, command[256], line[256], l[256], w[256];
    FILE *tfp,*f;

    tn = NULL;

    printf("Content-type: text/html%c%c",10,10);

    if(strcmp(getenv("CONTENT_TYPE"),"application/x-www-form-urlencoded")) {
        printf("This script can only be used to decode form results. \n");
        exit(1);
    }
    cl = atoi(getenv("CONTENT_LENGTH"));

    user=NULL;
    p1=NULL;
    p2=NULL;
    create=0;
    num = getnamevalue(&name, &value); 
    for(i=0; i < num; i++) {
        if(!strcmp(name[i],"user")) {
            if(!user)
                user = value[i];
            else {
                printf("This script was accessed from the wrong form.\n");
                exit(1);
            }
        }
        else if(!strcmp(name[i],"newpasswd1")) {
            if(!p1)
                p1 = value[i];
            else {
                printf("This script was accessed from the wrong form.\n");
                exit(1);
            }
        }
        else if(!strcmp(name[i],"newpasswd2")) {
            if(!p2)
                p2 = value[i];
            else {
                printf("This script was accessed from the wrong form.\n");
                exit(1);
            }
        }
        else {
            printf("This script was accessed from the wrong form.\n");
            printf("Unrecognized directive %s.\n",name[i]);
            exit(1);
        }
    }
    u=getenv("REMOTE_USER");
    if((strcmp(u,WIZARD)) && (strcmp(user,u))) {
            printf("<TITLE>User Mismatch</TITLE>");
            printf("<H1>User Mismatch</H1>");
            printf("The username you gave does not correspond with the ");
            printf("user you authenticated as.\n");
            exit(1);
        }
    if(strcmp(p1,p2)) {
        printf("<TITLE>Password Mismatch</TITLE>");
        printf("<H1>Password Mismatch</H1>");
        printf("The two copies of your the password do not match. Please");
        printf(" try again.");
        exit(1);
    }

    tn = tmpnam(NULL);
    tfp = fopen(tn,"w");
    if(!tfp) {
        fprintf(stderr,"Could not open temp file.\n");
        exit(1);
    }

    f = fopen(USER_FILE,"r");
    if(!f) {
        fprintf(stderr,
                "Could not open passwd file for reading.\n",USER_FILE);
        exit(1);
    }

    found = 0;
    while(!(getline(line,256,f))) {
        if(found || (line[0] == '#') || (!line[0])) {
            putline(tfp,line);
            continue;
        }
        strcpy(l,line);
        getword(w,l,':');
        if(strcmp(user,w)) {
            putline(tfp,line);
            continue;
        }
        else {
            change_password(user,p1,tfp);
            found=1;
        }
    }
    if((!found) && (create))
        change_password(user,p1,tfp);
    fclose(f);
    fclose(tfp);
    sprintf(command,"cp %s %s",tn,USER_FILE);
    system(command);
    remove(tn);
    printf("<TITLE>Successful Change</TITLE>");
    printf("<H1>Successful Change</H1>");
    printf("Your password has been successfully changed.<P>");
    delnamevalue(name, value, num); 
    exit(0);
}
