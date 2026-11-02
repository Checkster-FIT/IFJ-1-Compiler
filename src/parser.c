/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    parser.c
 * @brief   The code parser module responsible for syntax and semantic analysis of IFJ26 code
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
    parser->token_error = 0;

    parser->debug = true; // TODO: Disable debug
    parser->debug_prefix_failed = true;
    parser->debug_prefix_skipped = false;
    parser->debug_prefix_expression = false;
    // How many tokens after failure should be printed
    parser->debug_print_remaining = 7;
}

int destroy_parser(const ParserContext* parser)
{
    destroy_scanner(&parser->scanner_context);

    if (parser->token_error != 0)
        return parser->token_error;

    return parser->exit_code;
}

// =======================================================

int run_parser(SharedData* data)
{
    ParserContext parser;
    init_parser(&parser, data);

    next(&parser);
    const int status = parse_program(&parser);

    if (parser.debug && status == FAILED) {
        log_remaining(&parser);
    }

    return destroy_parser(&parser);
}

// =======================================================

int parse_program(ParserContext* parser)
{
    while (accept_next(LINE_FEED, parser)) {};

    if (accept_next_keyword(IMPORT_KW, parser)) {
        if (!parse_header(parser))
            return FAILED;
        return parse_body(parser);
    }

    parser->exit_code = print_error(PARSER_MISSING_HEADER, get_line(parser));
    return FAILED;
}

int parse_header(ParserContext* parser)
{
    if (!accept_next_identifier("ifj26", parser)) {
        parser->exit_code = print_error(PARSER_MISSING_HEADER, get_line(parser));
        return FAILED;
    }

    if (!accept_next(DOT, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_HEADER, get_line(parser));
        return FAILED;
    }

    if (!accept_next_operator(MULTIPLY, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_HEADER, get_line(parser));
        return FAILED;
    }

    return CORRECT;
}

int parse_body(ParserContext* parser)
{
    while (accept_next(LINE_FEED, parser)) {};

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

    if (accept_next(MAIN_IDENTIFIER, parser) && accept_next_keyword(DEF_KW, parser)) {
        if (!parse_function(parser))
            return FAILED;
        if (!parse_body(parser))
            return FAILED;
        return CORRECT;
    }

    if (accept_next(END_OF_FILE, parser)) {
        return CORRECT;
    }

    parser->exit_code = print_error(
        PARSER_GLOBAL_SCOPE_INVALID, get_line(parser),
        str_token(&parser->next, GET_ATTRIBUTE, parser->shared_data));
    return FAILED;
}

int parse_extension(ParserContext* parser)
{
    if (!accept_next(LEFT_BRACKET, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_EXT_PARAM, get_line(parser));
        return FAILED;
    }

    if (accept_next(IDENTIFIER, parser)) {
        if (!parse_param(parser))
            return FAILED;
    }

    if (!accept_next(RIGHT_BRACKET, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_EXT_PARAM_END, get_line(parser));
        return FAILED;
    }

    if (!accept_next(LINE_FEED, parser)) {
        parser->exit_code = print_error(PARSER_EXT_MISSING_NEWLINE, get_line(parser));
        return FAILED;
    }

    if (!accept_next(INDENT, parser)) {
        parser->exit_code = print_error(PARSER_EXT_MISSING_INDENT, get_line(parser));
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

    if (accept_next(DEDENT, parser)) {
        return CORRECT;
    }

    parser->exit_code = print_error(PARSER_EXT_BLOCK_INVALID, get_line(parser));
    return FAILED;
}

int parse_function(ParserContext* parser)
{
    if (!accept_next(IDENTIFIER, parser)) {
        parser->exit_code = print_error(PARSER_INVALID_FUNCTION, get_line(parser));
        return FAILED;
    }

    if (!parse_param_group_list(parser))
        return FAILED;

    if (!parse_type_specifier(parser))
        return FAILED;

    if (!accept_next(ASSIGNMENT, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_FUNCTION_ASSIGNMENT, get_line(parser));
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
    if (!accept_next_keyword(STRING_KW, parser) && !accept_next_keyword(INT_KW, parser)
        && !accept_next_keyword(DOUBLE_KW, parser) && !accept_next_keyword(UNIT_KW, parser)
        && !accept_next_keyword(BOOLEAN_KW, parser)) {
        parser->exit_code = print_error(
            PARSER_INCORRECT_TYPE, get_line(parser),
            str_token(&parser->next, GET_ATTRIBUTE, parser->shared_data));
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

    print_error(PARSER_INVALID_PARAM, get_line(parser));
    return FAILED;
}

int parse_param_list(ParserContext* parser)
{
    if (accept_next(RIGHT_BRACKET, parser)) {
        return CORRECT;
    }

    if (!accept_next(COMMA, parser)) {

        if (check_next(DECLARATION_SIGN, parser)) {
            parser->exit_code = print_error(PARSER_DOUBLE_PARAM_TYPE, get_line(parser));
            return FAILED;
        }

        parser->exit_code = print_error(PARSER_INVALID_PARAM_LIST_END, get_line(parser));
        return FAILED;
    }

    if (accept_next(IDENTIFIER, parser)) {
        if (!parse_param(parser))
            return FAILED;
        if (!parse_param_list(parser))
            return FAILED;
        return CORRECT;
    }

    print_error(PARSER_MISSING_PARAM_BRACKET, get_line(parser));
    return FAILED;
}

int parse_param(ParserContext* parser)
{
    if (!accept_next(DECLARATION_SIGN, parser)) {
        parser->exit_code = print_error(PARSER_MISSING_PARAM_TYPE, get_line(parser));
        return FAILED;
    }

    if (!parse_type(parser))
        return FAILED;

    return CORRECT;
}

int parse_function_body(ParserContext* parser)
{
    if (accept_next(LINE_FEED, parser)) {

        if (!accept_next(INDENT, parser)) {
            parser->exit_code = print_error(PARSER_FUNCTION_MISSING_INDENT, get_line(parser));
            return FAILED;
        }

        return parse_statement_list(parser);
    }

    if (!parse_expression(parser))
        return FAILED;

    if (!accept_next(LINE_FEED, parser)) {
        parser->exit_code
            = print_error(PARSER_FUNCTION_SINGLE_LINE_MISSING_LINE_FEED, get_line(parser));
        return FAILED;
    }

    return CORRECT;
}

int parse_statement_list(ParserContext* parser)
{
    if (accept_next(DEDENT, parser)) {
        return CORRECT;
    }

    if (!parse_statement(parser))
        return FAILED;

    return parse_statement_list(parser);
}

int parse_statement(ParserContext* parser)
{
    if (accept_next_keyword(VAR_KW, parser) || accept_next_keyword(VAL_KW, parser)) {
        if (!parse_statement_variable_definition(parser))
            return FAILED;
    } else if (accept_next_keyword(WHILE_KW, parser)) {
        if (!parse_statement_while(parser))
            return FAILED;
    } else if (accept_next_keyword(IF_KW, parser)) {
        if (!parse_statement_conditional(parser))
            return FAILED;
    } else if (accept_next_keyword(RETURN_KW, parser)) {
        if (!parse_statement_return(parser))
            return FAILED;
    } else if (accept_next_keyword(FOR_KW, parser)) {
        parser->exit_code = print_error(PARSER_INCOMPLETE_RULE, get_line(parser), "for_loop");
        return FAILED;
    } else if (
        check_next(LEFT_BRACKET, parser)
        || check_next(IDENTIFIER, parser)
        || check_next(STRING, parser)
        || check_next(STRING_INTERPOLATION, parser)
        || check_next(MULTILINE_STRING_INTERPOLATION, parser)
        || check_next(MULTILINE_STRING, parser)
        || check_next(INTEGER, parser)
        || check_next(DOUBLE, parser)
        || check_next(BOOLEAN, parser)
        ) {

        if (!parse_expression(parser))
            return FAILED;

        if (!accept_next(LINE_FEED, parser)) {
            parser->exit_code = print_error(PARSER_ST_MISSING_LINE_FEED, get_line(parser));
            return FAILED;
        }

    } else {
        return FAILED;
    }

    return CORRECT;
}

int parse_statement_conditional(ParserContext* parser)
{
    if (!parse_relational_expression(parser))
        return FAILED;

    if (!accept_next_keyword(THEN_KW, parser)) {
        parser->exit_code = print_error(PARSER_IF_MISSING_THEN, get_line(parser));
        return FAILED;
    }

    const bool indented = accept_next(LINE_FEED, parser);
    if (indented && !accept_next(INDENT, parser)) {
        parser->exit_code = print_error(PARSER_IF_MISSING_INDENT, get_line(parser));
        return FAILED;
    }

    if (!parse_statement_list(parser))
        return FAILED;

    if (!parse_else_branch(parser))
        return FAILED;

    return CORRECT;
}

int parse_else_branch(ParserContext* parser)
{
    if (accept_next(DEDENT, parser)) {
        return CORRECT;
    }

    if (!accept_next_keyword(ELSE_KW, parser)) {
        parser->exit_code = print_error(PARSER_COND_STATEMENT_UNCLOSED, get_line(parser));
        return FAILED;
    }

    const bool indented = accept_next(LINE_FEED, parser);
    if (indented && !accept_next(INDENT, parser)) {
        parser->exit_code = print_error(PARSER_IF_MISSING_INDENT, get_line(parser));
        return FAILED;
    }

    if (!parse_statement_list(parser))
        return FAILED;

    return CORRECT;
}

int parse_statement_while(ParserContext* parser)
{
    if (!parse_relational_expression(parser))
        return FAILED;

    if (!accept_next_keyword(DO_KW, parser)) {
        parser->exit_code = print_error(PARSER_WHILE_MISSING_DO, get_line(parser));
        return FAILED;
    }

    if (!accept_next(LINE_FEED, parser)) {
        parser->exit_code = print_error(PARSER_WHILE_MISSING_LINE_FEED, get_line(parser));
        return FAILED;
    }

    if (!accept_next(INDENT, parser)) {
        parser->exit_code = print_error(PARSER_WHILE_MISSING_INDENT, get_line(parser));
        return FAILED;
    }

    return parse_statement_list(parser);
}

int parse_statement_variable_definition(ParserContext* parser)
{
    if (!accept_next(IDENTIFIER, parser)) {
        parser->exit_code = print_error(PARSER_ST_VAR_DEF_MISSING_IDENTIFIER, get_line(parser));
        return FAILED;
    }

    if (!parse_type_specifier(parser))
        return FAILED;

    if (!accept_next(ASSIGNMENT, parser)) {
        parser->exit_code = print_error(PARSER_ST_VAR_DEF_MISSING_ASSIGNMENT, get_line(parser));
        return FAILED;
    }

    if (!parse_expression(parser))
        return FAILED;

    if (!accept_next(LINE_FEED, parser)) {
        parser->exit_code = print_error(PARSER_ST_MISSING_LINE_FEED, get_line(parser));
        return FAILED;
    }

    return CORRECT;
}

int parse_statement_return(ParserContext* parser)
{
    if (!parse_expression(parser))
        return FAILED;

    if (!accept_next(LINE_FEED, parser)) {
        parser->exit_code = print_error(PARSER_ST_MISSING_LINE_FEED, get_line(parser));
        return FAILED;
    }

    return CORRECT;
}

// =======================================================

int parse_relational_expression(ParserContext* parser)
{
    while (true) {
        if (check_next_keyword(DO_KW, parser) || check_next_keyword(THEN_KW, parser)) {
            return CORRECT;
        }

        parser->debug_prefix_skipped = true;
        next(parser);
    }

    parser->exit_code
        = print_error(PARSER_INCOMPLETE_RULE, get_line(parser), "relational_expression");
    return FAILED;
}

int parse_expression(ParserContext* parser)
{
    while (true) {
        if (check_next(LINE_FEED, parser) || check_next(END_OF_FILE, parser)) {
            return CORRECT;
        }

        parser->debug_prefix_skipped = true;
        next(parser);
    }

    parser->exit_code = print_error(PARSER_INCOMPLETE_RULE, get_line(parser), "expression");
    return FAILED;
}

// =======================================================

void next(ParserContext* parser)
{
    parser->accepted = parser->next;

    if (parser->debug) {
        log_parsed(parser);
    }

    parser->next = get_token(&parser->scanner_context, parser->shared_data, stdin);

    if (parser->next.type == ERROR) {
        parser->token_error = parser->next.attribute_error;
    }
}

bool check_next(const enum type_enum type, const ParserContext* parser)
{
    if (parser->next.type != type)
        return false;

    return true;
}

bool check_next_keyword(const enum attribute_keyword keyword, ParserContext* parser)
{
    if (check_next(KEYWORD, parser) == false)
        return false;

    if (parser->next.attribute_keyword != keyword)
        return false;

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
    if (check_next_keyword(keyword, parser) == false)
        return false;

    next(parser);
    return true;
}

bool accept_next_operator(const enum attribute_operation_type operation, ParserContext* parser)
{
    if (check_next(OPERATOR, parser) == false)
        return false;

    if (parser->next.attribute_operation_type != operation)
        return false;

    next(parser);
    return true;
}

// =======================================================

int get_line(const ParserContext* parser)
{
    if (parser->next.type == LINE_FEED)
        return parser->shared_data->line - 1;

    return parser->shared_data->line;
}

// =======================================================
