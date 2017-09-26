CC = gcc
CFLAGS = -g -Wall -I.
EXECS = master palin
OUTPUT_FILES = palin.out nopalin.out

all: $(EXECS)

clean:
	rm -f *.o $(EXECS) $(OUTPUT_FILES)
