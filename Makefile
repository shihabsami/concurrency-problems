CXX = g++
CXXFLAGS = -Wall -Werror -std=c++11 -g -pthread

readers_writers: main.cpp
	$(CXX) $(CXXFLAGS) -DREADERS_WRITERS -o simulation $<

sleeping_barbers: main.cpp
	$(CXX) $(CXXFLAGS) -DSLEEPING_BARBERS -o simulation $<

clean:
	$(RM) simulation *.o
