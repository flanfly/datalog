CXX = clang++
CXXARGS = -Wall -Werror -std=c++0x -g

all: dlog

%.o: %.cc $(wildcard *.hh)
	$(CXX) $(CXXARGS) -c -o $@ $<

dlog: dlog.o dsl.o
	$(CXX) -o  $@ $?
