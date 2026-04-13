## Makefile modified from the NCSA cgi-src distribution

CC     = gcc
CFLAGS = -g -O2

RM = rm -f

#--- You shouldn't have to edit anything else. ---

.c.o: 
	$(CC) -c $(CFLAGS) $<

all: form form1 env imagemap count change-passwd

ibm:
	make all CC=gcc

sun:
	make all CC=gcc
	#make all CC=gcc

hp: 
	make all CC=gcc

sgi:
	make all CC=cc

decmips:
	make all CC=cc

decaxp:
	make all CC=cc

tat: form form1 env
	$(RM) form.o form1.o env.o

env: env.o
	$(CC) env.o -o env

count: count.o
	$(CC) count.o -o count

imagemap: imagemap.o
	$(CC) imagemap.o -o imagemap

change-passwd: change-passwd.o util.c
	$(CC) change-passwd.o util.o -o change-passwd

form : form.o util.o
	$(CC) form.o util.o -o form

form1: form1.o util.o
	$(CC) form1.o util.o -o form1

clean:
	rm -f *.o env form form1 imagemap count change-passwd

