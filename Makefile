# Run `make all', `make spi', etc.

menu-test: menu-test.cpp menu.o
	g++ -std=c++11 -Wall -pthread menu-test.cpp menu.o -o menu-test -lncurses -lmenu

menu.o: menu.cpp menu.hpp
	g++ -c -std=c++11 -Wall -pthread menu.cpp -o menu.o


clean:
	rm -rf menu-test *.o
