main : Makefile main.cpp
	g++ -std=c++11 -pthread -Wall -Werror main.cpp -o main
.PHONY: run
run: main
	./main 3 in0 in1 in2 in3 in4