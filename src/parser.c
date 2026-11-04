/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    parser.c
 * @brief   The code parser module responsible for syntax and semantic analysis of IFJ26 code
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "parser.h"
#include "ast.h"
#include "debug.h"
#include "scanner.h"
#include "shared_data.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// =======================================================

void init_parser(ParserContext* parser, ASTNode* ast, SharedData* data)
{
    init_scanner(&parser->scanner_context);
    parser->shared_data = data;
    parser->ast = ast;

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

    // Return the scanner exit code if it failed
    if (parser->token_error != 0)
        return parser->token_error;

    return parser->exit_code;
}

// =======================================================

int run_parser(ASTNode* ast, SharedData* data)
{
    ParserContext parser;
    init_parser(&parser, ast, data);

    const int status = parse_program(&parser);

    if (parser.debug && status == FAILED) {
        log_remaining(&parser);
    }

    return destroy_parser(&parser);
}

// =======================================================

int parse_program(ParserContext* parser)
{
    next(parser);

    // <program> -> [LF] <program> (Discard any leading line feeds)
    while (accept_next(LINE_FEED, parser)) { };

    // <program> -> <header> <body>
    if (check_next_keyword(IMPORT_KW, parser)) {

        // <header>
        if (!parse_header(parser))
            return failed(parser, PARSER_MISSING_HEADER);

        // <body>
        return parse_body(parser);
    }

    return failed(parser, PARSER_MISSING_HEADER);
}

int parse_header(ParserContext* parser)
{
    // <header> -> import [IDENTIFIER] . *

    if (!accept_next_keyword(IMPORT_KW, parser))
        return FAILED;
    if (!accept_next_identifier("ifj26", parser))
        return FAILED;
    if (!accept_next(DOT, parser))
        return FAILED;
    if (!accept_next_operator(MULTIPLY, parser))
        return FAILED;

    return CORRECT;
}

int parse_body(ParserContext* parser)
{
    // <body> -> [LF] <body> (Discard any leading line feeds)
    while (accept_next(LINE_FEED, parser)) { };

    // <body> -> [EOF]
    if (accept_next(END_OF_FILE, parser))
        return CORRECT;

    // <body> -> <extension> <body>
    if (check_next_keyword(EXTENSION_KW, parser))
        return parse_extension(parser) && parse_body(parser);

    // <body> -> <function> <body>
    if (check_next_keyword(DEF_KW, parser))
        return parse_function(parser) && parse_body(parser);

    // <body> -> @main <function> <body>
    if (accept_next(MAIN_IDENTIFIER, parser) && check_next_keyword(DEF_KW, parser))
        return parse_function(parser) && parse_body(parser);

    const char* invalid = str_token(&parser->next, GET_ATTRIBUTE, parser->shared_data);
    return failed(parser, PARSER_GLOBAL_SCOPE_INVALID, invalid);
}

// -------------

int parse_extension(ParserContext* parser)
{
    // <extension> -> extension + ( + <param> + ) + [LF] + [INDENT] + <extension_body>

    if (!accept_next_keyword(EXTENSION_KW, parser))
        return FAILED;

    if (!accept_next(LEFT_BRACKET, parser))
        return failed(parser, PARSER_MISSING_EXT_PARAM);

    if (!parse_param(parser))
        return failed(parser, PARSER_MISSING_EXT_PARAM_END);

    if (!accept_next(RIGHT_BRACKET, parser))
        return failed(parser, PARSER_MISSING_EXT_PARAM_END);

    if (!accept_next(LINE_FEED, parser))
        return failed(parser, PARSER_EXT_MISSING_NEWLINE);

    if (!accept_next(INDENT, parser))
        return failed(parser, PARSER_EXT_MISSING_INDENT);

    return parse_extension_body(parser);
}

int parse_extension_body(ParserContext* parser)
{
    // <extension_body> -> [DEDENT]
    if (accept_next(DEDENT, parser))
        return CORRECT;

    // <extension_body> -> <function> + <extension_body>
    if (check_next_keyword(DEF_KW, parser))
        return parse_function(parser) && parse_extension_body(parser);

    return failed(parser, PARSER_EXT_BLOCK_INVALID);
}

// -------------

int parse_function(ParserContext* parser)
{
    // <function> -> def [IDENTIFIER] <param_group_list> <type_specifier> = <function_body>

    if (!accept_next_keyword(DEF_KW, parser))
        return FAILED;

    if (!accept_next(IDENTIFIER, parser))
        return failed(parser, PARSER_INVALID_FUNCTION);

    char* identifier = get_identifier(parser->shared_data, parser->accepted.attribute_str_idx);

    if (!parse_param_group_list(parser))
        return FAILED;

    if (!parse_type_specifier(parser))
        return FAILED;

    if (!accept_next(ASSIGNMENT, parser))
        return failed(parser, PARSER_MISSING_FUNCTION_ASSIGNMENT);

    if (add_function(parser->ast, identifier) == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR);
    log_ast_added(parser);

    return parse_function_body(parser);
}

int parse_type_specifier(ParserContext* parser)
{
    // <type_specifier> -> : <type>
    if (check_next(DECLARATION_SIGN, parser))
        return accept_next(DECLARATION_SIGN, parser) && parse_type(parser);

    // <type_specifier> -> 3
    return CORRECT;
}

int parse_type(ParserContext* parser)
{
    // <type> := String, Int, Double, Unit, Boolean
    if (
        accept_next_keyword(STRING_KW, parser)
        || accept_next_keyword(INT_KW, parser)
        || accept_next_keyword(DOUBLE_KW, parser)
        || accept_next_keyword(UNIT_KW, parser)
        || accept_next_keyword(BOOLEAN_KW, parser))
        return CORRECT;

    const char* invalid = str_token(&parser->next, GET_ATTRIBUTE, parser->shared_data);
    return failed(parser, PARSER_INCORRECT_TYPE, invalid);
}

int parse_param_group_list(ParserContext* parser)
{
    // <param_group_list> -> ( <param_list_first> ) <param_group_list>
    if (check_next(LEFT_BRACKET, parser)) {
        if (!accept_next(LEFT_BRACKET, parser))
            return FAILED;
        if (!parse_param_list_first(parser))
            return FAILED;
        if (!accept_next(RIGHT_BRACKET, parser))
            return failed(parser, PARSER_INVALID_PARAM_LIST_END);
        if (!parse_param_group_list(parser))
            return FAILED;

        return CORRECT;
    }

    // <param_group_list> -> 3
    return CORRECT;
}

int parse_param_list_first(ParserContext* parser)
{
    // <param_list_first> -> <param> <param_list>
    if (check_next(IDENTIFIER, parser))
        return parse_param(parser) && parse_param_list(parser);

    // <param_list_first> -> 3
    return CORRECT;
}

int parse_param_list(ParserContext* parser)
{
    // <param_list> -> , <param> <param_list>
    if (check_next(COMMA, parser))
        return accept_next(COMMA, parser) && parse_param(parser) && parse_param_list(parser);

    // <param_list> -> 3
    return CORRECT;
}

int parse_param(ParserContext* parser)
{
    // <param> -> [IDENTIFIER] : <type>

    if (!accept_next(IDENTIFIER, parser))
        return FAILED;

    if (!accept_next(DECLARATION_SIGN, parser))
        return failed(parser, PARSER_MISSING_PARAM_TYPE);

    if (!parse_type(parser))
        return FAILED;

    return CORRECT;
}

// -------------

int parse_function_body(ParserContext* parser)
{
    // <function_body> -> [LF] [INDENT] <statement_list> [DEDENT]
    if (check_next(LINE_FEED, parser)) {

        if (!accept_next(LINE_FEED, parser))
            return FAILED;

        if (!accept_next(INDENT, parser))
            return failed(parser, PARSER_FUNCTION_MISSING_INDENT);

        if (!parse_statement_list(parser))
            return FAILED;

        if (!accept_next(DEDENT, parser))
            return FAILED;

        return CORRECT;
    }

    // <function_body> -> <EXPR> [LF]
    if (parse_expression(parser)) {
        if (!accept_next(LINE_FEED, parser))
            return failed(parser, PARSER_FUNCTION_SINGLE_LINE_MISSING_LINE_FEED);
        return CORRECT;
    }

    return FAILED;
}

int parse_statement_list(ParserContext* parser)
{
    // <statement_list> -> 3
    if (check_next(DEDENT, parser)) // DEDENT from function body rule
        return CORRECT;

    // <statement_list> -> <construct> <statement_list>
    if (
        check_next_keyword(WHILE_KW, parser)
        || check_next_keyword(FOR_KW, parser)
        || check_next_keyword(IF_KW, parser)) {

        return parse_construct(parser) && parse_statement_list(parser);
    }

    // <statement_list> -> <statement> [LF] <statement_list>
    if (parse_statement(parser)) {

        if (!accept_next(LINE_FEED, parser)) {
            return failed(parser, PARSER_ST_MISSING_LINE_FEED);
        }

        return parse_statement_list(parser);
    }

    return FAILED;
}

// -------------

int parse_statement(ParserContext* parser)
{
    // <statement> -> <st_vardef>
    if (check_next_keyword(VAR_KW, parser) || check_next_keyword(VAL_KW, parser)) {
        if (!parse_statement_variable_definition(parser))
            return FAILED;
        return CORRECT;
    }
    // <statement> -> <st_return>
    if (check_next_keyword(RETURN_KW, parser)) {
        if (!parse_statement_return(parser))
            return FAILED;
        return CORRECT;
    }
    // <statement> -> <EXPR>
    return parse_expression(parser);
}

int parse_statement_variable_definition(ParserContext* parser)
{
    // <st_vardef> -> val [IDENTIFIER] <type_specifier> = <EXPR>
    // <st_vardef> -> var [IDENTIFIER] <type_specifier> = <EXPR>

    if (!accept_next_keyword(VAR_KW, parser) && !accept_next_keyword(VAL_KW, parser))
        return FAILED;

    if (!accept_next(IDENTIFIER, parser))
        return failed(parser, PARSER_ST_VAR_DEF_MISSING_IDENTIFIER);

    char* identifier = get_identifier(parser->shared_data, parser->accepted.attribute_str_idx);

    if (!parse_type_specifier(parser))
        return FAILED;

    if (!accept_next(ASSIGNMENT, parser))
        return failed(parser, PARSER_ST_VAR_DEF_MISSING_ASSIGNMENT);

    if (!parse_expression(parser))
        return FAILED;

    if (add_vardef(parser->ast, identifier) == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR);
    log_ast_added(parser);

    return CORRECT;
}

int parse_statement_return(ParserContext* parser)
{
    // <st_return> -> return <EXPR>
    return accept_next_keyword(RETURN_KW, parser) && parse_expression(parser);
}

// -------------

int parse_construct(ParserContext* parser)
{
    // <statement> -> <st_while>
    if (check_next_keyword(WHILE_KW, parser)) {
        if (!parse_construct_while(parser))
            return FAILED;
        return CORRECT;
    }
    // <statement> -> <st_conditional>
    if (check_next_keyword(IF_KW, parser)) {
        if (!parse_construct_if_then(parser))
            return FAILED;
        return CORRECT;
    }
    // <statement> -> <st_return>
    if (check_next_keyword(FOR_KW, parser)) {
        return failed(parser, PARSER_RULE_INCOMPLETE, "for_loop");
    }
    return FAILED;
}

int parse_construct_while(ParserContext* parser)
{
    // <c_while> -> while <RELATIONAL_EXPRESSION> do [LF] [INDENT] <statement_list> [DEDENT]

    if (!accept_next_keyword(WHILE_KW, parser))
        return FAILED;

    if (!parse_relational_expression(parser))
        return FAILED;

    if (!accept_next_keyword(DO_KW, parser))
        return failed(parser, PARSER_WHILE_MISSING_DO);

    if (!accept_next(LINE_FEED, parser))
        return failed(parser, PARSER_WHILE_MISSING_LINE_FEED);

    if (!accept_next(INDENT, parser))
        return failed(parser, PARSER_WHILE_MISSING_INDENT);

    if (!parse_statement_list(parser))
        return FAILED;

    if (!accept_next(DEDENT, parser))
        return FAILED;

    return CORRECT;
}

int parse_construct_if_then(ParserContext* parser)
{
    // <c_if_then> -> if <REL_EXPR> then [LF*] [INDENT*] <statement_list> [DEDENT*] <c_else>

    if (!accept_next_keyword(IF_KW, parser))
        return FAILED;

    if (!parse_relational_expression(parser))
        return FAILED;

    if (!accept_next_keyword(THEN_KW, parser))
        return failed(parser, PARSER_IF_MISSING_THEN);

    const bool indented = accept_next(LINE_FEED, parser);

    if (indented && !accept_next(INDENT, parser))
        return failed(parser, PARSER_IF_MISSING_INDENT);

    // TODO: Handle statements ending with else but not LF or ';'
    if (!parse_statement_list(parser))
        return FAILED;

    if (indented && !accept_next(DEDENT, parser))
        return FAILED;

    if (check_next_keyword(ELSE_KW, parser))
        return parse_construct_else(parser);

    return CORRECT;
}

int parse_construct_else(ParserContext* parser)
{
    // <c_else> -> else [LF*] [INDENT*] <statement_list> [DEDENT*]

    if (!accept_next_keyword(ELSE_KW, parser))
        return FAILED;

    const bool indented = accept_next(LINE_FEED, parser);

    if (indented && !accept_next(INDENT, parser))
        return failed(parser, PARSER_IF_MISSING_INDENT);

    if (!parse_statement_list(parser))
        return FAILED;

    if (indented && !accept_next(DEDENT, parser))
        return FAILED;

    return CORRECT;
}

// =======================================================

int parse_relational_expression(ParserContext* parser)
{
    while (true) {
        if (check_next_keyword(DO_KW, parser) || check_next_keyword(THEN_KW, parser))
            return CORRECT;

        if (check_next(LINE_FEED, parser) || check_next(END_OF_FILE, parser))
            return CORRECT;

        parser->debug_prefix_skipped = true;
        next(parser);
    }

    return failed(parser, PARSER_RULE_INCOMPLETE, "relational_expression");
}

int parse_expression(ParserContext* parser)
{
    while (true) {
        if (check_next(LINE_FEED, parser) || check_next(END_OF_FILE, parser))
            return CORRECT;

        parser->debug_prefix_skipped = true;
        next(parser);
    }

    return failed(parser, PARSER_RULE_INCOMPLETE, "expression");
}

// =======================================================

void next(ParserContext* parser)
{
    parser->accepted = parser->next;

    if (parser->debug)
        log_parsed(parser);

    parser->next = get_token(&parser->scanner_context, parser->shared_data, stdin);

    if (parser->next.type == ERROR)
        parser->token_error = parser->next.attribute_error;
}

// -------------------------------------------------------

bool check_next(const enum type_enum type, const ParserContext* parser)
{
    return parser->next.type == type;
}

bool check_next_keyword(const enum attribute_keyword keyword, const ParserContext* parser)
{
    if (!check_next(KEYWORD, parser))
        return false;

    return parser->next.attribute_keyword == keyword;
}

bool check_next_identifier(const char* name, const ParserContext* parser)
{
    if (!check_next(IDENTIFIER, parser))
        return false;

    const char* identifier = get_identifier(parser->shared_data, parser->next.attribute_str_idx);
    return (strcmp(identifier, name) == 0);
}

bool check_next_operator(const enum attribute_operation_type operation, const ParserContext* parser)
{
    if (!check_next(OPERATOR, parser))
        return false;

    return parser->next.attribute_operation_type == operation;
}

// -------------------------------------------------------

bool accept_next(const enum type_enum type, ParserContext* parser)
{
    if (!check_next(type, parser))
        return false;

    next(parser);
    return true;
}

bool accept_next_keyword(const enum attribute_keyword keyword, ParserContext* parser)
{
    if (!check_next_keyword(keyword, parser))
        return false;

    next(parser);
    return true;
}

bool accept_next_identifier(const char* name, ParserContext* parser)
{
    if (!check_next_identifier(name, parser))
        return false;

    next(parser);
    return true;
}

bool accept_next_operator(const enum attribute_operation_type operation, ParserContext* parser)
{
    if (!check_next_operator(operation, parser))
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

int failed(ParserContext* parser, const int err, ...)
{
    va_list args = { };
    va_start(args, err);

    parser->exit_code = va_print_error(err, get_line(parser), args);

    va_end(args);
    return FAILED;
}

// =======================================================
