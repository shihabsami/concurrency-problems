cc = GCC
CFLAGS = -Wall -std=c11 -pedantic -g -pthread

all: simulation

simulation: main.o
	$(CC) $(CFLAGS) -o $@ $<

main.o: main.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) simulation main.o
