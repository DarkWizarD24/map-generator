CFLAGS=-Wall -Wextra -std=c++11 -fdiagnostics-color=auto
SRCS=map.cpp

height_map.ppm: map
	./map

map: $(SRCS)
	g++ -I. $(CFLAGS) -o $@ $^

run: height_map.ppm

clean:
	@-rm *.ppm
	@-rm map
  
all: map run
  
.PHONY: all run
