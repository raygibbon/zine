CFLAGS = -O2 -Wall
CC     = gcc

name    = ../secrete.a
testsrc = bf-test.c rc4-test.c

all: testprogs

testprogs: $(testsrc)
	$(CC) $(CFLAGS) -c $(testsrc)
	$(CC) $(CFLAGS) -o bf-test.exe bf-test.o $(name)
	$(CC) $(CFLAGS) -o rc4-test.exe rc4-test.o $(name)

clean:
	rm -f bf-test.exe rc4-test.exe *.o

