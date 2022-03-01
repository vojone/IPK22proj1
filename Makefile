BIN = hinfosvc

SRC = server.c

INZIP = $(SRC) Readme.md Makefile

ZIPNAME = xdvora3o.zip

all: $(SRC)
	gcc -std=gnu99 -Wall -Wextra -pedantic $(SRC) -o $(BIN)

zip:
	zip $(ZIPNAME) $(INZIP) 

clean:
	rm -f $(BIN) $(ZIPNAME)
