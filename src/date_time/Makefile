
CXXFLAGS = -std=c++14 -g
CXX = clang++

all: a.out

date_time.o: date_time.hpp
main.o: date_time.hpp

a.out: main.o date_time.o
	$(CXX) -g main.o date_time.o

clean:
	rm -f *~ *.o date_time

.PHONY: clean
