# @author  Filip Hajdusek (xhajduf00)
# @brief   Project makefile with build, testing and other useful tools

# Compiler parameters
CC = gcc
CFLAGS = -Wall -Wextra
STANDARD = 17

# Project directory structure
SRC_DIR = src
TEST_DIR = tests
TEX_DIR = tex

# Project executable names
MAIN_EXEC = ifj26compiler

# Compilation database, used for project integration
DB_OUTPUT = compile_commands.json


ARCHIVE_NAME = archive
ARCHIVE_CONTAINS = src/* tests/* dokumentace.pdf Makefile rozdeleni

DOC_NAME = dokumentace

# -----------------------------------------------
.PHONY: build all run clean
# -----------------------------------------------

build: $(MAIN_EXEC)
all: $(MAIN_EXEC)

$(MAIN_EXEC): $(SRC_DIR)/*.c
	$(CC) $(CFLAGS) $^ -o $@

run: $(MAIN_EXEC)
	@./$(MAIN_EXEC)

valgrind: $(SRC_DIR)/*.c
	$(CC) $(CFLAGS) -g $^ -o $(MAIN_EXEC)
	valgrind --leak-check=full --show-leak-kinds=all ./$(MAIN_EXEC)

clean:
	@rm -fv $(MAIN_EXEC)
	@rm -frv $(BUILD)
	@rm -fv $(ARCHIVE_NAME)
	@rm -fv $(TEX_DIR)/*.aux
	@rm -fv $(TEX_DIR)/*.log
	@rm -fv *.pdf

# -----------------------------------------------
.PHONY: check format tidy checklist
# -----------------------------------------------
MAKEFLAGS += --no-print-directory

check:
	@echo && $(MAKE) checklist
	@echo && $(MAKE) format
	@echo && $(MAKE) tidy
	@echo

format:
	@command -v clang-format >/dev/null || { echo "[c] clang-format not installed!"; exit 0; }; \
	clang-format $(SRC_DIR)/*.c $(SRC_DIR)/*.h -i --verbose && \
	clang-format $(TEST_DIR)/*.cpp -i --verbose

tidy:
	@command -v clang-tidy >/dev/null ||  { echo "[c] clang-tidy not installed!"; exit 0; }; \
	clang-tidy $(SRC_DIR)/*.c -- -I$(SRC_DIR) -std=c$(STANDARD) && \
	clang-tidy $(TEST_DIR)/*.cpp -- -I$(SRC_DIR) -std=c++$(STANDARD)

checklist:
	@sh checklist.sh

# -----------------------------------------------
.PHONY: test build-test
# -----------------------------------------------

GTEST_FLAGS = -lgtest -pthread -g

# Units to be included in tests and the source files for tests
TEST_UNITS = scanner parser
TEST_UNITS_C = $(addsuffix .c, $(TEST_UNITS))
TEST_UNITS_O = $(addsuffix .o, $(TEST_UNITS))
TEST_UNITS_EXEC = $(addsuffix _tests, $(TEST_UNITS))

TEST_SHARED = error shared_data
TEST_SHARED_C = $(addsuffix .c, $(TEST_SHARED))
TEST_SHARED_O = $(addsuffix .o, $(TEST_SHARED))

GTEST_MAIN = $(TEST_DIR)/test_main.cpp

BUILD = $(TEST_DIR)/build
OBJ = $(BUILD)/obj

# Create build folder if necessary
$(BUILD):
	@mkdir -p $@

$(OBJ):
	@mkdir -p $@

# Create object file for a module
.SECONDARY:
$(OBJ)/%.o: $(SRC_DIR)/%.c | $(BUILD) $(OBJ)
	@gcc -I$(SRC_DIR) -c $< -o $@ -g -O0

# Create an executable for a test module
SHARED_OBJECTS = $(addprefix $(OBJ)/,$(TEST_SHARED_O))
$(BUILD)/%_tests: $(TEST_DIR)/%_tests.cpp $(OBJ)/%.o $(SHARED_OBJECTS) $(GTEST_MAIN)
	g++ -I$(SRC_DIR) $^ $(GTEST_FLAGS) -o $@

$(BUILD)/parser_tests: $(OBJ)/scanner.o

TEST_EXECUTABLES = $(addprefix $(BUILD)/,$(TEST_UNITS_EXEC))

build-test: $(TEST_EXECUTABLES)
test: $(TEST_EXECUTABLES)
	$^ 2> /dev/null || true

# -----------------------------------------------
.PHONY: database
# -----------------------------------------------
DB_TARGETS = build test

database:
	bear -- make -B $(DB_TARGETS)

# -----------------------------------------------
.PHONY: doc archive
# -----------------------------------------------

doc:
	cd $(TEX_DIR) && pdflatex $(DOC_NAME).tex
	mv $(TEX_DIR)/$(DOC_NAME).pdf $(DOC_NAME).pdf

archive: doc
	zip $(ARCHIVE_NAME).zip $(ARCHIVE_CONTAINS)

# -----------------------------------------------
.PHONY: interpret
# -----------------------------------------------

ic26int-linux-x86_64:
	scp merlin.fit.vut.cz:/pub/courses/ifj/ic26int/linux/ic26int ic26int
	chmod +x ic26int

interpret: ic26int-linux-x86_64
	./ic26int
