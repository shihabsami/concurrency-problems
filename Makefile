CXX = g++
CXXFLAGS = -Wall -Werror -std=c++17 -g -pthread

all: rw

rw: main.cpp
	$(CXX) $(CXXFLAGS) -DREADERS_WRITERS -o simulation $<

sb: main.cpp
	$(CXX) $(CXXFLAGS) -DSLEEPING_BARBERS -o simulation $<

rwc: main.cpp
	$(CXX) $(CXXFLAGS) -DREADERS_WRITERS -DCOLOUR_PRINT -o simulation $<

sbc: main.cpp
	$(CXX) $(CXXFLAGS) -DSLEEPING_BARBERS -DCOLOUR_PRINT -o simulation $<

clean:
	$(RM) simulation *.o
