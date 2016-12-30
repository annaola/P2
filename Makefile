main : Makefile main.cpp
	g++ -std=c++11 -pthread -Wall -Werror main.cpp -o main
