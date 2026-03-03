CC = gcc
CFLAGS = -Wall -Wextra -O2

PROG = mkamlimage unamlimage

all: $(PROG)

clean:
	rm -rf *.o $(PROG)
