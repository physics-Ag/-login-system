main: main.cc
	g++ main.cc -o main -std=c++11 -lpthread -ljsoncpp
clean:
	rm -f main
