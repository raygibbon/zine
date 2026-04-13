CC     = gcc
CFLAGS = -O2 -Wall

name   = ../secrete.a
libsrc = bf4.c rc4.c

all: secrete

secrete: $(libsrc)
	$(CC) -c $(CFLAGS) $^
	ar rs $(name) $(libsrc:.c=.o)

clean:
	rm -f *.o ${name}

