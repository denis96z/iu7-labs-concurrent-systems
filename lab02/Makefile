default: lab02

BIN_DIR  := $(PWD)/build
BIN_PATH := $(BIN_DIR)/lab02

.phony: lab02
lab02: dirs
	gcc main.c -fopenmp -o $(BIN_PATH)

.phony: dirs
dirs:
	mkdir -p $(BIN_DIR)

.phony: clean
clean:
	rm -f $(BIN_PATH)
