
FILES = $(wildcard *.c)
OBJECTS = $(subst .c,.o,$(FILES))

TARGET = dbtest
CC = gcc
CFLAGS = $(shell mysql_config --libs --include) -W -Wall

all:
	$(CC) -c $(CFLAGS) $(FILES)
	mv $(OBJECTS) $(OBJECTPATH)
dbtest: 
	$(CC) -c $(CFLAGS) $(FILES)
	$(CC) -o $(TARGET) $(OBJECTS) $(CFLAGS)

clean:
	rm -rf $(TARGET) $(OBJECTS)

