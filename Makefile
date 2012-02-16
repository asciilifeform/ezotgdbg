CFLAGS = -O2 -lusb
BIN = ezotgdbg

all :	$(BIN)

main : $(BIN).c
	$(CC) $(CFLAGS) -o $(BIN) $(BIN).c

install :
	cp ezotgdbg /usr/local/bin/

clean :
	rm -f nul core *.o $(BIN) *~

check-syntax:
	$(CC) -o nul -Wall -S $(CHK_SOURCES)
