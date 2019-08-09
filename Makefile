CC = gcc
CFLAGS = -I. -std=gnu99 -Wall -O2 -g
LFLAGS = -lm  -L. -ltiepie

ifeq ($(OS),Windows_NT)
  TARGET := streamspeedtest.exe
  RM := del
else
  TARGET := streamspeedtest
  RM := rm -f
endif

all : $(TARGET)

clean :
	$(RM) $(TARGET)

$(TARGET) : streamspeedtest.c
	$(CC) $(CFLAGS) -o $@ $< $(LFLAGS)
