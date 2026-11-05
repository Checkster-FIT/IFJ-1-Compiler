/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    parser.h
 * @brief   The code parser module responsible for syntax and semantic analysis of IFJ26 code
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_PARSER_H
#define IFJ_1_COMPILER_PARSER_H

#include "ast.h"
#include "scanner.h"
#include "symtable.h"
#include "shared_strings.h"
#include <stdbool.h>

#define CORRECT true
#define FAILED false

typedef struct {
    int exit_code;
    int token_error;
    Token next;
    Token accepted;

    ScannerContext scanner;

    Symtable* symtable;
    char last_type;

    // Debug information
    bool debug;
    bool debug_prefix_failed;
    bool debug_prefix_skipped;
    bool debug_prefix_expression;
    bool debug_ast_active;
    int debug_print_remaining;

    ASTNode* ast;
} ParserContext;

// -----------------------------------------
void init_parser(ParserContext* parser, ASTNode* ast, Symtable* symtable, AllocatedStrings* strings);
int run_parser(ASTNode* ast, Symtable* symtable, AllocatedStrings* strings);
int destroy_parser(const ParserContext* parser);

// -----------------------------------------
void next(ParserContext* parser);
bool check_next(enum type_enum type, const ParserContext* parser);
bool check_next_keyword(enum attribute_keyword keyword, const ParserContext* parser);
bool check_next_identifier(const char* name, const ParserContext* parser);
bool check_next_operator(enum attribute_operation_type operation, const ParserContext* parser);
bool accept_next(enum type_enum type, ParserContext* parser);
bool accept_next_keyword(enum attribute_keyword keyword, ParserContext* parser);
bool accept_next_identifier(const char* name, ParserContext* parser);
bool accept_next_operator(enum attribute_operation_type operation, ParserContext* parser);

// -----------------------------------------
int parse_program(ParserContext* parser);
int parse_header(ParserContext* parser);
int parse_body(ParserContext* parser);

int parse_extension(ParserContext* parser);
int parse_extension_body(ParserContext* parser);
int parse_function(ParserContext* parser);
int parse_function_body(ParserContext* parser);

int parse_statement_list(ParserContext* parser);

int parse_statement(ParserContext* parser);
int parse_statement_variable_definition(ParserContext* parser);
int parse_statement_return(ParserContext* parser);

int parse_construct(ParserContext* parser);
int parse_construct_while(ParserContext* parser);

int parse_construct_if_then(ParserContext* parser);
int parse_construct_else(ParserContext* parser);

int parse_type_specifier(ParserContext* parser);
int parse_type(ParserContext* parser);

int parse_param_group_list(ParserContext* parser);
int parse_param_list_first(ParserContext* parser);
int parse_param_list(ParserContext* parser);
int parse_param(ParserContext* parser);

int parse_expression(ParserContext* parser);
int parse_relational_expression(ParserContext* parser);

/**
 * Gets the correct line number adjusted for newlines
 * @param parser context with the line number and next token needed for the adjustment
 * @return the adjusted line number
 */
int get_line(const ParserContext* parser);

int failed(ParserContext* parser, int err, ...);

#endif // IFJ_1_COMPILER_PARSER_H