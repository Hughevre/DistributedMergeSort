SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

EXE := $(BIN_DIR)/MergeSortMPI
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CC       := mpicc
CPPFLAGS := -Iinc
CFLAGS   := -Wall -Wpedantic
LDFLAGS  :=
LDLIBS   := -lm
RUNNER   := mpiexec

NUM_ARRAY_SIZE := 8192
PROCESSES_NUM  := 8

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

run:
	$(RUNNER) -np $(PROCESSES_NUM) ./$(EXE) $(NUM_ARRAY_SIZE)

debug:
	$(RUNNER) -np $(PROCESSES_NUM) xterm -e gdb ./$(EXE)
	
clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

-include $(OBJ:.o=.d)