.PHONY : help build strip clean testjson testplain

VERSION=0.3

CFLAGS+=-D_GNU_SOURCE -std=c99 -Wall -Wextra -Werror -pipe
CFLAGS+=-Wno-unused-function
CFLAGS+=-ljansson
CFLAGS+=-O2
CFLAGS_DEBUG+=-g -DDEBUG

LIBS=

MAKE?=make
CC=gcc

default: build

help:
	@echo "----------------------------------------------------------------------"
	@echo "This Makefile handles the following targets:"
	@echo "     build  - builds fleece"
	@echo "build_debug - builds fleece with debug cflags"
	@echo "     strip  - strips the fleece binary"
	@echo "     clean  - cleans what has to be"
	@echo "  testjson  - runs a test with a json file on localhost port 12345 udp"
	@echo "  testncsa  - runs a test with a json to ncsa on localhost syslog"
	@echo "  testboth  - runs a both previous tests on localhoast port 12345"
	@echo "  testj2n   - runs a test with a bad json to ncsa"
	@echo "  json2ncsa - build a standalone version of json to ncsa like"
	@echo "----------------------------------------------------------------------"
	@echo "for tests : tcpdump -Xvelni lo port 12345"
	@echo "and (or)  : nc -u -l -s 127.0.0.1 -p 12345"

build: clean
	$(CC) $(CFLAGS) -o fleece src/fleece.c src/hostnameip.c src/json2ncsa.c

build_debug: clean
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) -o fleece src/fleece.c src/hostnameip.c src/json2ncsa.c

strip:
	strip -X fleece
	strip -s fleece

clean:
	rm -f fleece

testjson:
	@if [ -f fleece ]; \
	then \
		cat json.log.clean | ./fleece --host 127.0.0.1 --port 12345 --field pouet=lala --field tutu=tata \
	else \
		@echo "fleece binary was not found. Did you build it ?";\
	fi

testncsa:
	@if [ -f fleece ]; \
	then \
		cat json.log.clean | ./fleece --syslog-host 127.0.0.1 --syslog-port 12345 --field pouet=lala --field tutu=tata \
	else \
		@echo "fleece binary was not found. Did you build it ?";\
	fi

testboth:
	@if [ -f fleece ]; \
	then \
		cat json.log.clean | ./fleece --host 127.0.0.1 --port 12345 --syslog-host 127.0.0.1 --syslog-port 12345 --field pouet=lala --field tutu=tata \
	else \
		@echo "fleece binary was not found. Did you build it ?";\
	fi


testj2n:
	@if [ -f json2ncsa ]; \
	then \
		lines=`wc -l json.log.bad` \
		line=`cat json.log.bad | ./json2ncsa | wc -l` \
		if [ $lines != $line ];\
		then \
			@echo "test failed.";\
		else \
			@echo "test seems succeed.";\
		fi; \
	else \
		@echo "you have to build the standalone json2ncsa for this test";\
	fi

json2ncsa:
	$(CC) $(CFLAGS) -DSTANDALONE -o json2ncsa src/json2ncsa.c

