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
void log_ast_added(ParserContext* parser);

void print_ast(ASTNode* ast);
void print_ast_function(ASTNode* function_node, const Symtable* global_symtable);
void print_ast_parameter(const ASTNode* parameter_node, const Symtable* local_symtable);

void print_token(const Token* token);

const char* str_token(const Token* token, TokenStringType mode);
const char* str_keyword(int idx);
const char* str_operator(int idx);
const char* str_compare(int idx);

const char* map_str(const char** map, int size, int idx, const char* fallback);
void fprintf_escaped(FILE* stream, const char* string);

#endif // IFJ_1_COMPILER_DEBUG_H
