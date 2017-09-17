CC = gcc
CFLAGS = -g -Wall -I.
EXECS = master palin

all: $(EXECS)

clean:
	rm -f *.o $(EXECS)
