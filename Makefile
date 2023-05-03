CC=clang
CFLAGS=-g -Wall
BIN=httpserver

all:$(BIN)

$(BIN): httpserver.o
	$(CC) $^ -o $@

%.o: %.c %.h
	$(CC) -c $^

clean:
	rm *.o $(BIN)
