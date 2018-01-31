
CXXFLAGS = -std=c++17 -Wall -g -O3 -c

all: irc.cpp irc.hpp main.cpp
	clang++ $(CXXFLAGS) irc.cpp
	clang++ $(CXXFLAGS) main.cpp
	clang++ irc.o main.o -obot

clean:
	rm -f main.o irc.o bot
