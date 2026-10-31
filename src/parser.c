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

// =======================================================

void init_parser(ParserContext* parser, SharedData* data)
{
    init_scanner(&parser->scanner_context);

    parser->next.type = NONE;
    parser->accepted.type = NONE;
    parser->exit_code = 0;

    parser->shared_data = data;
}

int destroy_parser(const ParserContext* parser)
{
    destroy_scanner(&parser->scanner_context);
    return parser->exit_code;
    ;
}

int run_parser(SharedData* data)
{
    ParserContext parser;
    init_parser(&parser, data);

    const bool check_syntax = true;
    if (check_syntax) {
        next(&parser);
        parse_program(&parser);
    } else {
        while (true) {
            next(&parser);
            if (parser.next.type == END_OF_FILE || parser.next.type == ERROR)
                break;
        }
    }

    return destroy_parser(&parser);
}

// =======================================================

int parse_program(ParserContext* parser)
{
    if (accept_next(LINE_FEED, parser)) {
        return parse_program(parser);
    }

    if (accept_next_keyword(IMPORT_KW, parser)) {
        if (!parse_header(parser))
            return FAILED;
        if (!parse_body(parser))
            return FAILED;
    }

    if (accept_next(ERROR, parser))
        return FAILED;

    parser->exit_code = print_error(PARSER_MISSING_HEADER);
    return FAILED;
}

int parse_header(ParserContext* parser)
{
    if (!accept_next_identifier("ifj26", parser)) {
        parser->exit_code = print_error(PARSER_MISSING_HEADER);
        return FAILED;
    }

    if (!accept_next(DOT, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_HEADER);
        return FAILED;
    }

    if (!accept_next_operator(MULTIPLY, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_HEADER);
        return FAILED;
    }

    if (accept_next(ERROR, parser))
        return FAILED;

    return CORRECT;
}

int parse_body(ParserContext* parser)
{
    if (accept_next_keyword(DEF_KW, parser)) {
        if (!parse_function(parser))
            return FAILED;
        if (!parse_body(parser))
            return FAILED;
        return CORRECT;
    }

    if (accept_next_identifier("@main", parser) && accept_next_keyword(DEF_KW, parser)) {
        if (!parse_function(parser))
            return FAILED;
        if (!parse_body(parser))
            return FAILED;
        return CORRECT;
    }

    if (accept_next(END_OF_FILE, parser)) {
        return CORRECT;
    }

    if (accept_next(LINE_FEED, parser)) {
        return parse_body(parser);
    }

    parser->exit_code = print_error(PARSER_GLOBAL_SCOPE_INVALID, get_token_type_str(parser->next.type));
    return FAILED;
}

int parse_function(ParserContext* parser)
{
    parser->exit_code = print_error(PARSER_INVALID_FUNCTION);
    return FAILED;
}
// =======================================================

void next(ParserContext* parser)
{
    parser->accepted = parser->next;
    parser->next = get_token(&parser->scanner_context, parser->shared_data, stdin);

    print_token(&parser->next, parser->shared_data);
}

bool check_next(const enum type_enum type, ParserContext* parser)
{
    if (parser->next.type != type)
        return false;

    // Set the exit code if the scanner failed
    if (parser->next.type == ERROR)
        parser->exit_code = parser->next.attribute_error;

    return true;
}

bool accept_next(const enum type_enum type, ParserContext* parser)
{
    if (check_next(type, parser) == false)
        return false;

    next(parser);
    return true;
}

bool accept_next_identifier(const char* name, ParserContext* parser)
{
    if (check_next(IDENTIFIER, parser) == false)
        return false;

    const char* identifier = get_identifier(parser->shared_data, parser->next.attribute_str_idx);
    if (strcmp(identifier, name) != 0)
        return false;

    next(parser);
    return true;
}

bool accept_next_keyword(const enum attribute_keyword keyword, ParserContext* parser)
{
    if (check_next(KEYWORD, parser) == false)
        return false;

    if (parser->next.attribute_keyword != keyword)
        return false;

    next(parser);
    return true;
}

bool accept_next_operator(const enum attribute_operation_type operator, ParserContext* parser)
{
    if (check_next(OPERATOR, parser) == false)
        return false;

    if (parser->next.attribute_operation_type != operator)
        return false;

    next(parser);
    return true;
}

// =======================================================

