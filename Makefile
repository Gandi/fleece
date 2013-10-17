.PHONY : help build strip clean testjson testplain

VERSION=0.1

CFLAGS+=-D_POSIX_C_SOURCE=199309 -std=c99 -Wall -Wextra -Werror -pipe
CFLAGS+=-g
CFLAGS+=-Wno-unused-function
CFLAGS+=-ljansson
LIBS=

MAKE?=make
CC=gcc

CFLAGS+=-Iinclude
LDFLAGS+=-Llib

default: build

help:
	@echo "----------------------------------------------------------------------"
	@echo "This Makefile handles the following targets:"
	@echo "     build  - builds fleece"
	@echo "     strip  - strips the fleece binary"
	@echo "     clean  - cleans what has to be"
	@echo "  testjson  - runs a test with a json file on localhost port 12345 udp"
	@echo "  testplain - runs a test with a string on localhost port 12345 udp"
	@echo "----------------------------------------------------------------------"
	@echo "for tests : tcpdump -Xvelni lo port 12345"

build: clean
	$(CC) $(CFLAGS) -o fleece src/fleece.c src/str.c src/hostnameip.c

clean:
	rm -f fleece

testjson:
	@if [ -f fleece ]; \
	then \
		cat json.log.clean | ./fleece --host 127.0.0.1 --port 12345 --field pouet=lala --field tutu=tata \
	else \
		@echo "fleece binary was not found. Did you build it ?";\
	fi

testplain:
	@if [ -f fleece ]; \
	then \
		echo "pouetpouet" | ./fleece --host 127.0.0.1 --port 12345 --field pouet=lala --field tutu=tata \
	else \
		@echo "fleece binary was not found. Did you build it ?";\
	fi
