.POSIX:
.SUFFIXES:
PREFIX=/usr/local
IDIR=include
SDIR=src
LIBDIR=lib
CC=gcc
CFLAGS=-I$(IDIR) -Wall -D_POSIX_C_SOURCE=200809L -DINCLUDE_MAIN \
       -std=c17 -Wno-unused-function \
	$(shell pkg-config --cflags glib-2.0)
DEBUG_FLAGS= -DDEBUG -g3 -Wextra -pedantic -Wdouble-promotion \
	     -Wno-unused-parameter -Wno-sign-conversion \
	     -fsanitize=undefined,address -fsanitize-undefined-trap-on-error
RELEASE_FLAGS=-O3 -flto
LDLIBS = -llmdb $(shell pkg-config --libs glib-2.0)

C_FILES = pdjson.c database.c util.c platformdep.c deinflector.c
H_FILES = pdjson.h database.h util.h platformdep.h deinflector.h
SRC = $(addprefix $(SDIR)/,$(C_FILES))
SRC_H = $(addprefix $(IDIR)/,$(H_FILES))

default: release

release: $(SRC) $(SRC_H)
	$(CC) -o jppron $(SDIR)/jppron.c $(CFLAGS) $(RELEASE_FLAGS) $(LDLIBS) $(SRC)
debug: $(SRC) $(SRC_H)
	$(CC) -o jppron $(SDIR)/jppron.c $(CFLAGS) $(DEBUG_FLAGS) $(LDLIBS) $(SRC)

install:
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f jppron ${DESTDIR}${PREFIX}/bin

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/jppron

clean:
	rm -f jppron

.PHONY: clean install uninstall
