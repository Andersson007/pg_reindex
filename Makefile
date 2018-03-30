CC=gcc
#OPTIMIZATION=-O3
OPTIMIZATION=-O0
DEBUG=
STANDARD=-std=c11
WARN_LEVEL=-Wall
CFLAGS=-I /usr/pgsql-9.6/include/ -c $(WARN_LEVEL) $(DEBUG) $(STANDARD) $(OPTIMIZATION)
LDLIBS=-L /usr/pgsql-9.6/lib -lpq
LDFLAGS=

VPATH=
SOURCES=$(wildcard *.c)
HEADERS=headers/$(wildcard *.h)
OBJECTS=$(patsubst %.c,%.o, $(SOURCES))
EXECUTABLE=pg_reindex

SCP=scp -r
REMOTE_HOST=
REMOTE_USER=
REMOTE_DIR=
FILES=*

CLEAR=rm -rf

.c.o:
	$(CC) $(CFLAGS) $< -o $@

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDLIBS) $(OBJECTS) -o $@

$(OBJECTS): $(HEADERS)

.PHONY: clean scp

clean:
	$(CLEAR) $(OBJECTS) $(EXECUTABLE)

scp:
	$(SCP) $(FILES) $(REMOTE_USER)@$(REMOTE_HOST):$(REMOTE_DIR)

