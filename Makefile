# @author  Filip Hajdusek (xhajduf00)
# @brief   Project makefile with build, testing and other useful tools

# Compiler parameters
CC = gcc
CFLAGS = -Wall -Wextra -g -O0
CXXFLAGS += -std=gnu++17
STANDARD = 17

# Project directory structure
SRC_DIR = src
TEST_DIR = tests
TEST_DATA_DIR = $(TEST_DIR)/data
TEX_DIR = tex

# Project executable names
MAIN_EXEC = ifj26compiler

# Compilation database, used for project integration
DB_OUTPUT = compile_commands.json

ARCHIVE_NAME = xhajduf00
ARCHIVE_CONTAINS = src/* tests/* tests/data/* dokumentace.pdf Makefile rozdeleni rozsireni
ARCHIVE_EXCLUDES = tests/data/* tests/data/*/* tests/build/* tests/build/*/* tests/compile

DOC_NAME = dokumentace

CODE_EXAMPLE = $(TEST_DATA_DIR)/basic.ifj26
CODE_CUSTOM_EXAMPLE = $(TEST_DATA_DIR)/custom.ifj26

# -----------------------------------------------
.PHONY: build all run valgrind clean
# -----------------------------------------------

build: $(MAIN_EXEC)
all: $(MAIN_EXEC)

$(MAIN_EXEC): $(SRC_DIR)/*.c
	$(CC) $(CFLAGS) $^ -o $@

run: $(MAIN_EXEC)
	@./$(MAIN_EXEC) < $(CODE_EXAMPLE)

# -----------------------------------------------
.PHONY: custom strings factorial boolthen control lambdas algebdt
.PHONY: extkw funexp interpolateds paramlists valmem lists
# -----------------------------------------------

custom basic: $(MAIN_EXEC)
	@./$(MAIN_EXEC) < $(TEST_DATA_DIR)/$@.ifj26

tree: $(MAIN_EXEC)
	@./$(MAIN_EXEC) -t < $(TEST_DATA_DIR)/$@.ifj26

strings factorial: $(MAIN_EXEC)
	@./$(MAIN_EXEC) < $(TEST_DATA_DIR)/core/$@.ifj26

boolthen control lambdas algebdt extkw lists funexp interpolateds paramlists valmem: $(MAIN_EXEC)
	@./$(MAIN_EXEC) < $(TEST_DATA_DIR)/extensions/$@.ifj26


valgrind: $(SRC_DIR)/*.c
	$(CC) $(CFLAGS) -g $^ -o $(MAIN_EXEC)
	valgrind --leak-check=full --show-leak-kinds=all ./$(MAIN_EXEC) < $(CODE_CUSTOM_EXAMPLE)

clean:
	@rm -fv $(MAIN_EXEC)
	@rm -frv $(BUILD)
	@rm -fv $(ARCHIVE_NAME)
	@rm -fv $(TEX_DIR)/*.aux
	@rm -fv $(TEX_DIR)/*.log
	@rm -fv *.pdf
	@rm -fv *.log
	@rm -fv $(ARCHIVE_NAME).zip

# -----------------------------------------------
.PHONY: check format tidy tidy-cpp checklist examples
# -----------------------------------------------
MAKEFLAGS += --no-print-directory

check:
	@$(MAKE) checklist
	@echo && $(MAKE) format
	@echo && $(MAKE) tidy
	@echo

format:
	@clang-format $(SRC_DIR)/*.c $(SRC_DIR)/*.h -i --verbose
	@clang-format $(TEST_DIR)/*.cpp -i --verbose

tidy:
	@clang-tidy $(SRC_DIR)/*.c -- -I$(SRC_DIR) -std=c$(STANDARD)

tidy-cpp:
	@clang-tidy $(TEST_DIR)/*.cpp -- -I$(SRC_DIR) -std=c++$(STANDARD)

checklist:
	@bash checklist.sh

examples: $(MAIN_EXEC)
	@bash ./tests/data/compile.sh

# -----------------------------------------------
.PHONY: test build-test
# -----------------------------------------------

GTEST_FLAGS = -lgtest -pthread -g

# Units to be included in tests and the source files for tests
TEST_UNITS = scanner parser
TEST_UNITS_C = $(addsuffix .c, $(TEST_UNITS))
TEST_UNITS_H = $(addsuffix .h, $(TEST_UNITS))
TEST_UNITS_O = $(addsuffix .o, $(TEST_UNITS))
TEST_UNITS_EXEC = $(addsuffix _tests, $(TEST_UNITS))

TEST_COMPLEX = analysis runtime
TEST_COMPLEX_EXEC = $(addsuffix _tests, $(TEST_UNITS))

TEST_SHARED = error shared_strings symtable ast bst debug stack
TEST_SHARED_C = $(addsuffix .c, $(TEST_SHARED))
TEST_SHARED_H = $(addsuffix .h, $(TEST_SHARED))
TEST_SHARED_O = $(addsuffix .o, $(TEST_SHARED))

GTEST_MAIN = $(TEST_DIR)/test_main.cpp

BUILD = $(TEST_DIR)/build
OBJ = $(BUILD)/obj

# Create build folder if necessary
$(BUILD):
	@mkdir -p $@

$(OBJ):
	@mkdir -p $@

# Add the generated .d files as dependencies
-include $(wildcard $(OBJ)/*.d)

# Compile each module
$(OBJ)/%.o: $(SRC_DIR)/%.c | $(BUILD) $(OBJ)
	gcc -I$(SRC_DIR) -MMD -MP -c $< -o $@ -g -O0

# Compile each test source file
$(OBJ)/%_tests.o: $(TEST_DIR)/%_tests.cpp | $(BUILD) $(OBJ)
	$(CXX) -I$(SRC_DIR) -MMD -MP -c $< -o $@ -g

# Compile the shared test main
$(OBJ)/test_main.o: $(GTEST_MAIN) | $(BUILD) $(OBJ)
	$(CXX) -I$(SRC_DIR) -MMD -MP -c $< -o $@ -g

# Create an executable for each test module
SHARED_OBJECTS = $(addprefix $(OBJ)/,$(TEST_SHARED_O))
TEST_MAIN_OBJECT = $(OBJ)/test_main.o

# Link the object files together
$(BUILD)/%_tests: $(OBJ)/%_tests.o $(OBJ)/%.o $(SHARED_OBJECTS) $(TEST_MAIN_OBJECT)
	$(CXX) $^ $(GTEST_FLAGS) -o $@

$(BUILD)/analysis_tests: $(OBJ)/analysis_tests.o \
	$(SHARED_OBJECTS) \
	$(OBJ)/parser.o \
	$(OBJ)/scanner.o \
	$(OBJ)/semantic.o \
	$(TEST_MAIN_OBJECT)
	$(CXX) $^ $(GTEST_FLAGS) -o $@

$(BUILD)/runtime_tests: $(OBJ)/runtime_tests.o \
	$(SHARED_OBJECTS) \
	$(OBJ)/parser.o \
	$(OBJ)/scanner.o \
	$(OBJ)/semantic.o \
	$(OBJ)/generator.o \
	$(TEST_MAIN_OBJECT)
	$(CXX) $^ $(GTEST_FLAGS) -o $@

# [!] ANY ADDITIONAL TEST DEPENDENCIES GO HERE
$(BUILD)/parser_tests: $(OBJ)/scanner.o
$(BUILD)/scanner_tests: $(OBJ)/parser.o
$(BUILD)/analysis_tests: $(OBJ)/parser.o $(OBJ)/scanner.o $(OBJ)/semantic.o

TEST_EXECUTABLES = \
	$(addprefix $(BUILD)/,$(addsuffix _tests,$(TEST_UNITS))) \
	$(addprefix $(BUILD)/,$(addsuffix _tests,$(TEST_COMPLEX)))

TEST_EXECUTABLES_LIST = \
	$(addsuffix _tests,$(TEST_UNITS)) \
	$(addsuffix _tests,$(TEST_COMPLEX))

build-test: $(TEST_EXECUTABLES) $(MAIN_EXEC)
test: $(TEST_EXECUTABLES) $(MAIN_EXEC)
	@cd $(BUILD) && for test in $(TEST_EXECUTABLES_LIST); do echo; ./$$test 2>/dev/null || true; done

test-parser-build: $(BUILD)/parser_tests
test-parser: $(BUILD)/parser_tests
	@$^ 2>/dev/null || true

test-scanner-build: $(BUILD)/scanner_tests
test-scanner: $(BUILD)/scanner_tests
	@$^ 2>/dev/null || true

test-analysis-build: $(BUILD)/analysis_tests
test-analysis: $(BUILD)/analysis_tests
	@$^ 2>/dev/null || true

test-runtime-build: $(BUILD)/runtime_tests $(MAIN_EXEC) $(INTERPRET)
	chmod +x $(INTERPRET)

test-runtime: $(BUILD)/runtime_tests $(MAIN_EXEC) $(INTERPRET)
	@cd $(BUILD) && ./runtime_tests 2>/dev/null || true

# -----------------------------------------------
.PHONY: database
# -----------------------------------------------
DB_TARGETS = build test

database:
	bear -- make -B $(DB_TARGETS)

# -----------------------------------------------
.PHONY: doc archive ok
# -----------------------------------------------

doc:
	cd $(TEX_DIR) && pdflatex $(DOC_NAME).tex
	mv $(TEX_DIR)/$(DOC_NAME).pdf $(DOC_NAME).pdf

ok: $(ARCHIVE_NAME).zip
	@chmod +x is_it_ok.sh
	@./is_it_ok.sh $(ARCHIVE_NAME).zip

archive: $(ARCHIVE_NAME).zip
$(ARCHIVE_NAME).zip: doc
	zip -rD $(ARCHIVE_NAME).zip $(ARCHIVE_CONTAINS) -x $(ARCHIVE_EXCLUDES)

# -----------------------------------------------
.PHONY: interpret
# -----------------------------------------------

INTERPRET = ic26int-linux-x86_64

interpret: $(INTERPRET)
$(INTERPRET):
	scp merlin.fit.vut.cz:/pub/courses/ifj/ic26int/linux/$(INTERPRET) $(INTERPRET)
	chmod +x $(INTERPRET)
