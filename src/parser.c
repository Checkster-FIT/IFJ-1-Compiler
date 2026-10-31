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
    parser->shared_data = data;

    parser->next.type = NONE;
    parser->next.attribute_none = NULL;
    parser->accepted.type = NONE;
    parser->accepted.attribute_none = NULL;
    parser->exit_code = 0;

    // Print debug token list
    parser->debug = false;
    // Highlight where the parser failed with [!?]
    parser->debug_highlight_failed = true;
    // How many tokens after failure should be printed
    parser->debug_print_remaining = 8;
}

int destroy_parser(const ParserContext* parser)
{
    destroy_scanner(&parser->scanner_context);
    return parser->exit_code;
}

// =======================================================

int run_parser(SharedData* data)
{
    ParserContext parser;
    init_parser(&parser, data);
    parser.debug = true;

    next(&parser);
    parse_program(&parser);

    if (parser.debug) {

        while (parser.next.type != END_OF_FILE && parser.next.type != ERROR) {
            next(&parser);
            parser.debug_highlight_failed = false;
            parser.debug_print_remaining--;
        }

        if (parser.debug_print_remaining < 0)
            fprintf(stderr, "[**] %i more remaining\n", -(parser.debug_print_remaining));
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
        return parse_body(parser);
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
    if (accept_next_keyword(EXTENSION_KW, parser)) {
        if (!parse_extension(parser))
            return FAILED;
        if (!parse_body(parser))
            return FAILED;
        return CORRECT;
    }

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

int parse_extension(ParserContext* parser)
{
    if (!accept_next(LEFT_BRACKET, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_EXT_PARAM);
        return FAILED;
    }

    if (accept_next(IDENTIFIER, parser)) {
        if (!parse_param(parser))
            return FAILED;
    }

    if (!accept_next(RIGHT_BRACKET, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_EXT_PARAM_END);
        return FAILED;
    }

    if (!accept_next(LINE_FEED, parser)) {
        parser->exit_code = print_error(PARSER_EXT_MISSING_NEWLINE);
        return FAILED;
    }

    if (!accept_next(INDENT, parser)) {
        parser->exit_code = print_error(PARSER_EXT_MISSING_INDENT);
        return FAILED;
    }

    return parse_extension_body(parser);
}

int parse_extension_body(ParserContext* parser)
{
    if (accept_next_keyword(DEF_KW, parser)) {
        if (!parse_function(parser))
            return FAILED;
        if (!parse_extension_body(parser))
            return FAILED;
        return CORRECT;
    }

    // TODO: Change to dedent
    if (accept_next_keyword(INDENT, parser)) {
        return CORRECT;
    }

    parser->exit_code = print_error(PARSER_EXT_BLOCK_INVALID);
    return FAILED;
}

int parse_function(ParserContext* parser)
{
    if (!accept_next(IDENTIFIER, parser)) {
        parser->exit_code = print_error(PARSER_INVALID_FUNCTION);
        return FAILED;
    }

    if (!parse_param_group_list(parser))
        return FAILED;

    if (!parse_type_specifier(parser))
        return FAILED;

    if (!accept_next(ASSIGNMENT, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_FUNCTION_ASSIGNMENT);
        return FAILED;
    }

    if (!parse_function_body(parser))
        return FAILED;

    return CORRECT;
}

int parse_type_specifier(ParserContext* parser)
{
    if (accept_next(DECLARATION_SIGN, parser)) {
        if (!parse_type(parser))
            return FAILED;
    }

    return CORRECT;
}

int parse_type(ParserContext* parser)
{
    if (!accept_next_keyword(STRING_KW, parser) && !accept_next_keyword(INT_KW, parser) && !accept_next_keyword(DOUBLE_KW, parser) && !accept_next_keyword(UNIT_KW, parser) && !accept_next_keyword(BOOLEAN_KW, parser)) {
        parser->exit_code = print_error(PARSER_INCORRECT_TYPE);
        return FAILED;
    }

    return CORRECT;
}

int parse_param_group_list(ParserContext* parser)
{
    if (accept_next(LEFT_BRACKET, parser)) {
        if (!parse_param_first(parser))
            return FAILED;
        if (!parse_param_group_list(parser))
            return FAILED;
    }

    return CORRECT;
}

int parse_param_first(ParserContext* parser)
{
    if (accept_next(RIGHT_BRACKET, parser)) {
        return CORRECT;
    }

    if (accept_next(IDENTIFIER, parser)) {
        if (!parse_param(parser))
            return FAILED;
        if (!parse_param_list(parser))
            return FAILED;
        return CORRECT;
    }

    print_error(PARSER_INVALID_PARAM);
    return FAILED;
}

int parse_param_list(ParserContext* parser)
{
    if (accept_next(RIGHT_BRACKET, parser)) {
        return CORRECT;
    }

    if (!accept_next(COMMA, parser)) {

        if (check_next(DECLARATION_SIGN, parser)) {
            parser->exit_code = print_error(PARSER_DOUBLE_PARAM_TYPE);
            return FAILED;
        }

        parser->exit_code = print_error(PARSER_INVALID_PARAM_LIST_END);
        return FAILED;
    }

    if (accept_next(IDENTIFIER, parser)) {
        if (!parse_param(parser))
            return FAILED;
        if (!parse_param_list(parser))
            return FAILED;
        return CORRECT;
    }

    print_error(PARSER_MISSING_PARAM_BRACKET);
    return FAILED;
}

int parse_param(ParserContext* parser)
{
    if (!accept_next(DECLARATION_SIGN, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_PARAM_TYPE);
        return FAILED;
    }

    if (!parse_type(parser))
        return FAILED;

    return CORRECT;
}

int parse_function_body(ParserContext* parser)
{
    parser->exit_code = print_error(PARSER_INCOMPLETE_RULE);
    return FAILED;
}

// =======================================================

void next(ParserContext* parser)
{
    parser->accepted = parser->next;
    parser->next = get_token(&parser->scanner_context, parser->shared_data, stdin);

    if (parser->debug && parser->accepted.type != NONE) {
        // Accepted token
        if (parser->exit_code == 0) {
            fprintf(stderr, "[\033[1;32mOK\033[0m] ");
            print_token(&parser->accepted, parser->shared_data);
        }
        // First invalid token
        else if (parser->debug_highlight_failed) {
            fprintf(stderr, "[\033[1;31m!?\033[0m] ");
            print_token(&parser->accepted, parser->shared_data);
        }
        // Unprocessed tokens after
        else if (parser->debug_print_remaining > 0) {
            fprintf(stderr, "[  ] ");
            print_token(&parser->accepted, parser->shared_data);
        }
    }
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
