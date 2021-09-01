CXX = g++
CXXFLAGS = -Wall -Werror -std=c++11 -g -pthread

all: simulation

simulation: sleeping_barber.o
	$(CXX) $(CXXFLAGS) -o $@ $<

sleeping_barber.o: sleeping_barber.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	$(RM) simulation *.o
