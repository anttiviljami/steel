CC = gcc
CFLAGS = $(shell grep '^CFLAGS = ' crypt_blowfish/Makefile | cut -d= -f2-)
.PHONY: crypt_blowfish

all: bcrypt.a

test: bcrypt.c crypt_blowfish
	$(CC) $(CFLAGS) -c bcrypt.c
	$(CC) -o bcrypt.o crypt_blowfish/*.o

bcrypt.a: bcrypt.o crypt_blowfish
	ar r libbcrypt.a bcrypt.o crypt_blowfish/*.o

bcrypt.o: bcrypt.c
	$(CC) $(CFLAGS) -c bcrypt.c

crypt_blowfish:
	$(MAKE) -C crypt_blowfish

clean:
	rm -f *.o libbcrypt.a *~ core
	$(MAKE) -C crypt_blowfish clean
