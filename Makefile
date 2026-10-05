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
TEST_EXEC = ifj26tests

# Units to be included in tests and the source files for tests
TEST_UNITS = scanner.c
TEST_FILES = test.cpp

# Compilation database, used for project integration
DB_OUTPUT = compile_commands.json
DB_TARGETS = build build-tests

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
	@rm -fv $(TEST_EXEC)
	@rm -fv $(ARCHIVE_NAME)
	@rm -fv $(TEX_DIR)/*.aux
	@rm -fv $(TEX_DIR)/*.log
	@rm -fv *.pdf

# -----------------------------------------------
.PHONY: database
# -----------------------------------------------
database:
	bear -- make -B $(DB_TARGETS)

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
.PHONY: build-tests test
# -----------------------------------------------

GTEST_OBJ = ./tests/testing.o
GTEST_FLAGS = -lgtest -lgtest_main -pthread

$(TEST_EXEC): $(TEST_DIR)/*.cpp $(SRC_DIR)/*.c
	@gcc -Isrc -c src/$(TEST_UNITS) -o $(GTEST_OBJ)
	@g++ -Isrc tests/$(TEST_FILES) $(GTEST_OBJ) $(GTEST_FLAGS) -o $(TEST_EXEC)
	@rm -f $(GTEST_OBJ)

test: $(TEST_EXEC)
	@./ifj26tests

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
