# Run `make all', `make spi', etc.
all: main.cpp
	g++ main.cpp -o prog -lmenu -lncurses

spi-test: spi.o leds.o spi-test.cpp
	g++ spi-test.cpp spi.o leds.o -o spi-test -lwiringPi

spi.o: spi.cpp spi.hpp
	g++ -c spi.cpp -o spi.o

leds.o: leds.cpp leds.hpp
	g++ -c leds.cpp -o leds.o

menu.o: menu.cpp menu.hpp 
	g++ -std=c++11 -Wall -c menu.cpp -o menu.o

menu-test: menu-test.cpp menu.o
	g++ -std=c++11 -Wall -pthread -I/usr/include/eigen3/ menu-test.cpp menu.o -o menu-test -lncurses -lmenu

maths.o: 
	g++ -std=c++11 math_test.cpp consts.o -o math_test

classes.o: classes.cpp consts.hpp
	g++ -std=c++11 -I/usr/include/eigen3/ -c consts.cpp -o consts.o

static_test:
	g++ -std=c++11 -Wall -c static_test_2.cpp -o static_test_2.o
	g++ -std=c++11 -Wall static_test_1.cpp static_test_2.o -o static

clean:
	rm -rf prog spi-test menu-test *.o
