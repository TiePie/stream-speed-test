CC = gcc
CFLAGS = -std=gnu99 -Wall -O2 -g
LFLAGS = -lm -ltiepie

all : streamspeedtest

clean :
	rm -f streamspeedtest

streamspeedtest : streamspeedtest.c
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS)
