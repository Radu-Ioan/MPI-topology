CFLAGS = -Wall -std=c++17 -g
CC = mpic++

BIN = tema3

EXTRA_SOURCES = $(wildcard processing/*.cpp)

OBJECTS = $(patsubst %.cpp,%.o,$(EXTRA_SOURCES))

build: $(BIN)

$(BIN): main.cpp $(OBJECTS)
	$(CC) -o $@ $^ $(CFLAGS)

.cpp.o:
	$(CC) -o $@ $< $(CFLAGS) -c

clean:
	rm -rf $(OBJECTS) $(BIN) *.o
