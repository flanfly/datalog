CXX = clang++
CXXARGS = -Wall -Werror -std=c++0x -g

all: test

%.o: %.cc $(wildcard *.hh)
	$(CXX) $(CXXARGS) -c -o $@ $<

test: dlog.o dsl.o test.o
	$(CXX) -lcppunit -o $@ $^
