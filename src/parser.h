/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    parser.h
 * @brief   The code parser module responsible for syntax and semantic analysis of IFJ26 code
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_PARSER_H
#define IFJ_1_COMPILER_PARSER_H
#include "scanner.h"
#include "shared_data.h"
#include <stdbool.h>

#define CORRECT true
#define FAILED false

typedef struct {
    int exit_code;
    Token next;
    Token accepted;
    ScannerContext scanner_context;
    SharedData* shared_data;

    // Debug information
    bool debug;
    bool debug_highlight_failed;
    bool debug_skipped;
    int debug_print_remaining;

} ParserContext;

// -----------------------------------------
void init_parser(ParserContext* parser, SharedData* data);
int run_parser(SharedData* data);
int destroy_parser(const ParserContext* parser);

// -----------------------------------------
void next(ParserContext* parser);
bool check_next(const enum type_enum type, ParserContext* parser);
bool check_next_keyword(const enum attribute_keyword keyword, ParserContext* parser);
bool accept_next(const enum type_enum type, ParserContext* parser);
bool accept_next_keyword(const enum attribute_keyword keyword, ParserContext* parser);
bool accept_next_identifier(const char* name, ParserContext* parser);
bool accept_next_operator(const enum attribute_operation_type operator, ParserContext * parser);

// -----------------------------------------
int parse_program(ParserContext* parser);
int parse_header(ParserContext* parser);
int parse_body(ParserContext* parser);

int parse_extension(ParserContext* parser);
int parse_extension_body(ParserContext* parser);
int parse_function(ParserContext* parser);
int parse_function_body(ParserContext* parser);

int parse_statement_list(ParserContext* parser);
int parse_statement_variable_definition(ParserContext* parser);
int parse_statement_while(ParserContext* parser);
int parse_statement_return(ParserContext* parser);

int parse_statement_conditional(ParserContext* parser);
int parse_if_then(ParserContext* parser);
int parse_else_if_list(ParserContext* parser);
int parse_else(ParserContext* parser);

int parse_type_specifier(ParserContext* parser);
int parse_type(ParserContext* parser);

int parse_param_group_list(ParserContext* parser);
int parse_param_first(ParserContext* parser);
int parse_param_list(ParserContext* parser);
int parse_param(ParserContext* parser);

int parse_expression(ParserContext* parser);
int parse_relational_expression(ParserContext* parser);

#endif // IFJ_1_COMPILER_PARSER_H