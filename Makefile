cc = GCC
CFLAGS = -Wall -std=c11 -pedantic -g -pthread

all: simulation

simulation: readers_writers.o
	$(CC) $(CFLAGS) -o $@ $<

readers_writers.o: readers_writers.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) simulation *.o
