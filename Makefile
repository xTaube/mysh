BIN=bin
EXEC=$(BIN)/mysh
SRC=src/*.c

$(BIN):
mkdir -p $(BIN)

$(EXEC): $(BIN) $(SRC)
	gcc --std=c2x -Wall -Iinclude -o $(EXEC) $(SRC)

run: $(EXEC)
	./$(EXEC)
