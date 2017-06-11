OBJECTPATH = $(shell pwd)/object
export OBJECTPATH

CFLAGS = $(shell mysql_config --include --libs) -pthread -W -Wall -g -ggdb
export CFLAGS

CC = gcc
FILES = $(wildcard *.c)
OBJECTS = $(subst .c,.o,$(FILES))

DIRS = ./cacao_DB ./server 

TARGET = test

all:
	$(CC) -c $(FILES) $(CFLAGS)
	mv $(OBJECTS) $(OBJECTPATH)
	
	@for dir in $(DIRS); do\
		make -C $$dir;\
	done
	
	$(CC) -o $(TARGET) $(OBJECTPATH)/*.o $(CFLAGS)

clean:
	rm -rf $(TARGET) $(OBJECTPATH)/*.o

