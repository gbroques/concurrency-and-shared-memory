CC=gcc
CFLAGS=-I.
OBJ=master.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

master: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o master
