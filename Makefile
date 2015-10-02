CC=gcc
override CFLAGS+=-std=c99 -Wall
PREFIX=/usr/local
LDFLAGS=

all: steel

steel: steel.o
	$(CC) $(CFLAGS) steel.o -o steel $(LDFLAGS)

steel.o: steel.c
	$(CC) $(CFLAGS) -c steel.c

clean:
	rm steel
	rm *.o

install: all
	if [ ! -d $(PREFIX)/share/man/man1 ];then	\
		mkdir -p $(PREFIX)/share/man/man1;	\
	fi
	cp memo.1 $(PREFIX)/share/man/man1/
	gzip -f $(PREFIX)/share/man/man1/steel.1
	cp memo $(PREFIX)/bin/

uninstall:
	rm $(PREFIX)/bin/steel
	rm $(PREFIX)/share/man/man1/steel.1.gz

