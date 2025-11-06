BIN=bin
EXEC=$(BIN)/mysh
SRC=src/*.c

$(BIN):
	mkdir -p $(BIN)

$(EXEC): $(BIN) $(SRC)
	gcc --std=c2x -Wall -Iinclude $(OPTS) -o $(EXEC) $(SRC)

run: $(EXEC)
	./$(EXEC)
