CC      = gcc
CFLAGS  = -m32 -Wall -Wextra -std=c11
# CFLAGS  = -Wall -Wextra -std=c11

all: hexeditplus

hexeditplus: hexeditplus.o
	$(CC) $(CFLAGS) -o hexeditplus hexeditplus.o

hexeditplus.o: hexeditplus.c
	$(CC) $(CFLAGS) -c hexeditplus.c -o hexeditplus.o

clean:
	rm -f hexeditplus.o hexeditplus
