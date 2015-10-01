CC=gcc
override CFLAGS+=-std=c99 -Wall
PREFIX=/usr/local
LDFLAGS=

all: fort

fort: fort.o
	$(CC) $(CFLAGS) fort.o -o fort $(LDFLAGS)

fort.o: fort.c
	$(CC) $(CFLAGS) -c fort.c

clean:
	rm fort
	rm *.o

install: all
	if [ ! -d $(PREFIX)/share/man/man1 ];then	\
		mkdir -p $(PREFIX)/share/man/man1;	\
	fi
	cp memo.1 $(PREFIX)/share/man/man1/
	gzip -f $(PREFIX)/share/man/man1/fort.1
	cp memo $(PREFIX)/bin/

uninstall:
	rm $(PREFIX)/bin/fort
	rm $(PREFIX)/share/man/man1/fort.1.gz

