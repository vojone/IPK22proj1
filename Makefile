BIN = hinfosvc

SRC = server.c

all:
	gcc -std=gnu99 -Wall -Wextra -pedantic $(SRC) -o $(BIN)
