/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    parser.h
 * @brief
 * @author
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
} ParserContext;

// -----------------------------------------
void init_parser(ParserContext* parser, SharedData* data);
int run_parser(SharedData* data);
int destroy_parser(const ParserContext* parser);

// -----------------------------------------
void next(ParserContext* parser);
bool check_next(const enum type_enum type, ParserContext* parser);
bool accept_next(const enum type_enum type, ParserContext* parser);

// -----------------------------------------
bool accept_next_identifier(const char* name, ParserContext* parser);
bool accept_next_keyword(const enum attribute_keyword keyword, ParserContext* parser);
bool accept_next_operator(const enum attribute_operation_type operator, ParserContext* parser);

// -----------------------------------------
int parse_header(ParserContext* parser);
int parse_function(ParserContext* parser);
int parse_body(ParserContext* parser);
int parse_program(ParserContext* parser);

#endif // IFJ_1_COMPILER_PARSER_H