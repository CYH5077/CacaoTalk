FILES = $(wildcard *.c)
OBJECTS = $(subst .c,.o,$(FILES))
CC = gcc

all:
	$(CC) -c $(CFLAGS) $(FILES)
	mv $(OBJECTS) $(OBJECTPATH)
