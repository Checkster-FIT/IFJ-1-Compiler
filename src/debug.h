/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    debug.h
 * @brief   Various debug and logging functionality
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_DEBUG_H
#define IFJ_1_COMPILER_DEBUG_H

#include "parser.h"
#include "scanner.h"

typedef enum {
    GET_NAME,
    GET_ATTRIBUTE
} TokenStringType;

#define ALIGNMENT 35
#define BUFFER_SIZE 32

void log_parsed(ParserContext* parser);
void log_remaining(ParserContext* parser);

void log_ast_start(ParserContext* parser);
void log_ast_end(ParserContext* parser);

typedef struct branch {
    const struct branch* parent;
    Symtable* symtable;
    bool followed;
    bool last;
} Branch;

void print_ast(ASTNode* root);
void print_ast_indent(const Branch* branch);
int print_ast_recursively(ASTNode* node, Branch* parent);
const SymtableEntry* get_scoped_symtable_entry(const Branch* branch, const char* identifier);

void log_precedence_stack(const ParserContext* parser, const Stack* stack);

void print_token(const Token* token);

void print_line(ParserContext* parser);

bool is_string_token(const Token* token);
const char* str_operation(const Operator op);

const char* str_type(const char type_char);
const char* str_token(const Token* token, TokenStringType mode);
const char* str_keyword(int idx);
const char* str_token_operator(int idx);
const char* str_compare(int idx);

const char* map_str(const char** map, int size, int idx, const char* fallback);
void fprintf_escaped(FILE* stream, const char* string);

#endif // IFJ_1_COMPILER_DEBUG_H
