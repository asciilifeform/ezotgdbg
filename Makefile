CFLAGS = -O2 -lusb
BIN = ezotgdbg

all :	$(BIN)

main : $(BIN).c
	$(CC) $(CFLAGS) -o $(BIN) $(BIN).c

clean :
	rm -f nul core *.o $(BIN) *~

check-syntax:
	$(CC) -o nul -Wall -Wextra -pedantic -S $(CHK_SOURCES)
