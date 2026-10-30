/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    parser.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "parser.h"
#include "debug.h"
#include "scanner.h"
#include "shared_data.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define CORRECT (0)
#define FAILED (-1)

typedef struct {
    int exit_code;
    ScannerContext scanner_context;
    SharedData* shared_data;
} ParserContext;

void init_parser(ParserContext* parser, SharedData* data)
{
    init_scanner(&parser->scanner_context);
    parser->shared_data = data;
}

void destroy_parser(ParserContext* parser)
{
    destroy_scanner(&parser->scanner_context);
}

enum error_enum next_token(Token* token, ParserContext* parser)
{
    *token = get_token(&parser->scanner_context, parser->shared_data, stdin);
    print_token(token, parser->shared_data);

    if (token->type == ERROR)
        return FAILED;

    return CORRECT;
}

// -----------------------------------------

int parse_header(ParserContext* parser)
{
    Token token;

    if (next_token(&token, parser) != CORRECT) {
        parser->exit_code = token.attribute_error;
        return FAILED;
    }

    // TODO: Add ifj26 identifier name check
    if (token.type != IDENTIFIER) {
        parser->exit_code = print_error(PARSER_SYNTAX_ERROR, "ifj26.*", get_token_type_str(token.type));
        return FAILED;
    }

    if (next_token(&token, parser) != CORRECT) {
        parser->exit_code = token.attribute_error;
        return FAILED;
    }

    if (token.type != DOT) {
        parser->exit_code = print_error(PARSER_SYNTAX_ERROR, "ifj26.*", get_token_type_str(token.type));
        return FAILED;
    }

    if (next_token(&token, parser) != CORRECT) {
        parser->exit_code = token.attribute_error;
        return FAILED;
    }

    if (token.type != OPERATOR || token.attribute_operation_type != MULTIPLY) {
        parser->exit_code = print_error(PARSER_SYNTAX_ERROR, "ifj26.*", get_token_type_str(token.type));
        return FAILED;
    }

    return CORRECT;
}

int parse_main_function(ParserContext* parser)
{
    parser->exit_code = parser->exit_code;
    return CORRECT;
}

int parse_function(ParserContext* parser)
{
    parser->exit_code = parser->exit_code;
    return CORRECT;
}

int parse_body(ParserContext* parser)
{
    Token token;

    if (next_token(&token, parser) != CORRECT) {
        parser->exit_code = token.attribute_error;
        return FAILED;
    }

    if (token.type == IDENTIFIER && strcmp(get_identifier(parser->shared_data, token.attribute_str_idx), "@main") == 0) {
        if (parse_main_function(parser) != CORRECT)
            return FAILED;
        if (parse_body(parser) != CORRECT)
            return FAILED;
        return CORRECT;
    }

    if (token.type == KEYWORD && token.attribute_keyword == DEF_KW) {
        if (parse_function(parser) != CORRECT)
            return FAILED;
        if (parse_body(parser) != CORRECT)
            return FAILED;
        return CORRECT;
    }

    if (token.type == END_OF_FILE) {
        return CORRECT;
    }

    if (token.type == LINE_FEED) {
        if (parse_body(parser) != CORRECT)
            return FAILED;
        return CORRECT;
    }

    parser->exit_code = print_error(PARSER_GLOBAL_SCOPE_INVALID, get_token_type_str(token.type));
    return FAILED;
}

int parse_program(ParserContext* parser)
{
    Token token;

    if (next_token(&token, parser) != CORRECT)
        return token.attribute_error;

    if (token.type == LINE_FEED) {
        if (parse_program(parser) != CORRECT)
            return FAILED;
        return CORRECT;
    }

    if (token.type == KEYWORD && token.attribute_keyword == IMPORT_KW) {
        if (parse_header(parser) != CORRECT)
            return FAILED;
        if (parse_body(parser) != CORRECT)
            return FAILED;
        return CORRECT;
    }

    parser->exit_code = print_error(PARSER_MISSING_HEADER);
    return FAILED;
}

// -----------------------------------------

int run_parser(SharedData* data)
{
    ParserContext parser;
    init_parser(&parser, data);

    const bool check_syntax = true;

    if (check_syntax) {
        parse_program(&parser);
    } else {
        Token token;
        while (next_token(&token, &parser) == CORRECT) {
            if (token.type == ERROR || token.type == END_OF_FILE)
                break;
        }
    }

    destroy_parser(&parser);
    return 0;
}
