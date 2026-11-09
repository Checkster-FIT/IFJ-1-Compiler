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
#include "shared_strings.h"
#include "symtable.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// =======================================================

ParserContext* init_parser(ParserContext* parser, ASTNode* ast, AllocatedStrings* strings)
{
    // Scanner initialization
    if (init_scanner(&parser->scanner, strings) == 1)
        return NULL;

    parser->prev_line = -1;
    parser->future_line = -1;

    // Symtable stack initialization
    parser->symtable_stack = init_stack();
    if (parser->symtable_stack == NULL) {
        destroy_scanner(&parser->scanner);
        return NULL;
    }

    parser->ast = ast;
    push(parser->symtable_stack, ast->program.global_symtable);

    // Scanned tokens
    parser->future.type = NONE;
    parser->future.attribute_none = NULL;
    parser->next.type = NONE;
    parser->next.attribute_none = NULL;
    parser->accepted.type = NONE;
    parser->accepted.attribute_none = NULL;

    parser->exit_code = 0;
    parser->token_error = 0;

    parser->parameter_expression = false;
    parser->interpolated_expression = false;
    parser->expressions_nested = 0;

    // Semantic analysis
    parser->last_type = '?';
    parser->inside_extension = false;

    // Debug settings
    parser->debug = true; // TODO: Disable debug
    parser->debug_stack = (parser->debug) && true;
    parser->debug_ast_nodes = (parser->debug) && true;
    parser->debug_indent_terms = (parser->debug) && true;

    // Temporary prefix flags
    parser->debug_prefix_failed = true;
    parser->debug_prefix_skipped = false;
    parser->debug_prefix_term = false;
    parser->debug_prefix_ast = false;

    // How many tokens after failure should be printed
    parser->debug_print_remaining = 7;

    return parser;
}

int destroy_parser(const ParserContext* parser)
{
    destroy_scanner(&parser->scanner);
    dispose_stack(parser->symtable_stack);

    // Return the scanner exit code if it failed
    if (parser->token_error != 0)
        return parser->token_error;

    return parser->exit_code;
}

// =======================================================

int run_parser(ASTNode* ast, AllocatedStrings* strings)
{
    ParserContext parser;
    if (init_parser(&parser, ast, strings) == NULL)
        return print_error(MEMORY_ALLOCATION_ERROR, 0, "ParserContext");

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
    if (check_next_keyword(EXTENSION_KW, parser)) {
        parser->inside_extension = true;

        if (!parse_extension(parser))
            return FAILED;

        parser->inside_extension = false;
        return parse_body(parser);
    }

    // <body> -> <function> <body>
    // <body> -> @main <function> <body>
    if (check_next_keyword(DEF_KW, parser) || check_next(MAIN_IDENTIFIER, parser))
        return parse_function(parser) && parse_body(parser);

    const char* invalid = str_token(&parser->next, GET_ATTRIBUTE);
    return failed(parser, PARSER_GLOBAL_SCOPE_INVALID, invalid);
}

// -------------

int parse_extension(ParserContext* parser)
{
    log_ast_start(parser);
    // <extension> -> extension + ( + <param> + ) + [LF] + [INDENT] + <extension_body>

    if (!accept_next_keyword(EXTENSION_KW, parser))
        return FAILED;

    // - Create a new local symtable
    Symtable* local_symtable = new_symtable();
    if (local_symtable == NULL || push(parser->symtable_stack, local_symtable))
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Local symtable");

    // AST - New extension
    if (ast_add_extension(parser->ast, local_symtable) == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST extension");
    log_ast_end(parser);

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
    if (accept_next(DEDENT, parser)) {
        pop(parser->symtable_stack);
        return CORRECT;
    }

    // <extension_body> -> <function> + <extension_body>
    if (check_next_keyword(DEF_KW, parser))
        return parse_function(parser) && parse_extension_body(parser);

    return failed(parser, PARSER_EXT_BLOCK_INVALID);
}

// -------------

int parse_function(ParserContext* parser)
{
    log_ast_start(parser);
    // <function> -> def [IDENTIFIER] <param_group_list> <type_specifier> = <function_body>

    const bool is_main = accept_next(MAIN_IDENTIFIER, parser);

    if (is_main)
        accept_next(LINE_FEED, parser);

    if (!accept_next_keyword(DEF_KW, parser))
        return FAILED;

    if (!accept_next(IDENTIFIER, parser))
        return failed(parser, PARSER_INVALID_FUNCTION);

    SymtableEntry* function = symtable_add_entry(top(parser->symtable_stack), (SymtableEntry) { .defined = true, .main = is_main, .identifier = parser->accepted.attribute_string });

    // - Create a new local symtable
    Symtable* local_symtable = new_symtable();
    if (local_symtable == NULL || push(parser->symtable_stack, local_symtable))
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Local symtable");

    // AST - New function
    if (parser->inside_extension) {
        if (ast_add_ext_function(parser->ast, function->identifier, local_symtable) == NULL)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Function");
    } else {
        if (ast_add_function(parser->ast, function->identifier, local_symtable) == NULL)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Function");
    }
    log_ast_end(parser);

    const bool tmp = parser->inside_extension;
    parser->inside_extension = false;

    if (!parse_param_group_list(parser))
        return FAILED;

    parser->inside_extension = tmp;

    if (!parse_type_specifier(parser))
        return FAILED;

    function->data_type = parser->last_type;

    if (!accept_next(ASSIGNMENT, parser))
        return failed(parser, PARSER_MISSING_FUNCTION_ASSIGNMENT);

    if (!parse_function_body(parser))
        return FAILED;

    pop(parser->symtable_stack);
    return CORRECT;
}

int parse_type_specifier(ParserContext* parser)
{
    // <type_specifier> -> : <type>
    if (check_next(DECLARATION_SIGN, parser))
        return accept_next(DECLARATION_SIGN, parser) && parse_type(parser);

    // <type_specifier> -> 3
    parser->last_type = '?';
    return CORRECT;
}

int parse_type(ParserContext* parser)
{
    // <type> := String, Int, Double, Unit, Boolean
    if (accept_next_keyword(STRING_KW, parser)) {
        parser->last_type = 's';
        return CORRECT;
    }
    if (accept_next_keyword(INT_KW, parser)) {
        parser->last_type = 'i';
        return CORRECT;
    }
    if (accept_next_keyword(DOUBLE_KW, parser)) {
        parser->last_type = 'd';
        return CORRECT;
    }
    if (accept_next_keyword(UNIT_KW, parser)) {
        parser->last_type = 'u';
        return CORRECT;
    }
    if (accept_next_keyword(BOOLEAN_KW, parser)) {
        parser->last_type = 'b';
        return CORRECT;
    }

    const char* invalid = str_token(&parser->next, GET_ATTRIBUTE);
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
    log_ast_start(parser);

    if (!accept_next(IDENTIFIER, parser))
        return FAILED;

    SymtableEntry parameter = {
        .defined = true,
        .identifier = parser->accepted.attribute_string,
        .data_type = '?'
    };

    if (!accept_next(DECLARATION_SIGN, parser))
        return failed(parser, PARSER_MISSING_PARAM_TYPE);

    if (!parse_type(parser))
        return FAILED;

    parameter.data_type = parser->last_type;

    // AST - Parameter
    if (parser->inside_extension) {
        if (ast_add_ext_parameter(parser->ast, parameter.identifier) == NULL)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Parameter");
    } else {
        if (ast_add_parameter(parser->ast, parameter.identifier) == NULL)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Parameter");
    }
    log_ast_end(parser);

    // Symtable - Parameter
    symtable_add_entry(top(parser->symtable_stack), parameter);
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
    if (parse_expression(parser, NULL)) {
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

        if (!parse_separator(parser)) {
            return failed(parser, PARSER_ST_MISSING_SEPARATOR);
        }

        return parse_statement_list(parser);
    }

    return FAILED;
}

int parse_separator(ParserContext* parser)
{
    // <separator> -> [LF]
    if (accept_next(LINE_FEED, parser))
        return CORRECT;

    if (accept_next(SEMICOLON, parser)) {

        // <separator> -> ; [LF]
        accept_next(LINE_FEED, parser);

        // <separator> -> ;
        return CORRECT;
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
    // <st_assign> -> [IDENTIFIER] = <EXPR>
    if (check_next(IDENTIFIER, parser) && check_future(ASSIGNMENT, parser)) {
        if (!parse_statement_assignment(parser))
            return FAILED;
        return CORRECT;
    }

    // <statement> -> <EXPR>
    return parse_expression(parser, NULL);
}

int parse_statement_variable_definition(ParserContext* parser)
{
    // <st_vardef> -> val [IDENTIFIER] <type_specifier> = <EXPR>
    // <st_vardef> -> var [IDENTIFIER] <type_specifier> = <EXPR>
    log_ast_start(parser);
    SymtableEntry variable = {
        .defined = true,
        .data_type = '?'
    };

    if (!accept_next_keyword(VAR_KW, parser) && !accept_next_keyword(VAL_KW, parser))
        return FAILED;
    variable.constant = parser->accepted.attribute_keyword == VAL_KW;

    if (!accept_next(IDENTIFIER, parser))
        return failed(parser, PARSER_ST_VAR_DEF_MISSING_IDENTIFIER);
    variable.identifier = parser->accepted.attribute_string;

    if (!parse_type_specifier(parser))
        return FAILED;
    variable.data_type = parser->last_type;
    symtable_add_entry(top(parser->symtable_stack), variable);

    if (!accept_next(ASSIGNMENT, parser))
        return failed(parser, PARSER_ST_VAR_DEF_MISSING_ASSIGNMENT);

    ASTNode* node = ast_add_vardef(parser->ast, variable.identifier);
    if (node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST VarDef");

    if (!parse_expression(parser, (ASTExpression**)&(node->vardef.expression)))
        return FAILED;

    log_ast_end(parser);

    return CORRECT;
}

int parse_statement_assignment(ParserContext* parser)
{
    // <st_assign> -> [IDENTIFIER] = <EXPR>
    if (!accept_next(IDENTIFIER, parser))
        return FAILED;

    if (!accept_next(ASSIGNMENT, parser))
        return FAILED;

    return parse_expression(parser, NULL);
}

int parse_statement_return(ParserContext* parser)
{
    // <st_return> -> return <EXPR>
    return accept_next_keyword(RETURN_KW, parser) && parse_expression(parser, NULL);
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

    if (!parse_expression(parser, NULL))
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

    if (!parse_expression(parser, NULL))
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

int parse_term(ParserContext* parser)
{
    int status = FAILED;

    const bool tmp_term_debug = parser->debug_prefix_term;
    parser->debug_prefix_term = true;

    // TODO: Temporary basic term accepting for now
    //  Add support for more complex terms

    // <term> -> [INTEGER] <dot_call>
    if (accept_next(INTEGER, parser))
        status = parse_dot_call(parser);

    // <term> -> [DOUBLE] <dot_call>
    else if (accept_next(DOUBLE, parser))
        status = parse_dot_call(parser);

    // <term> -> [BOOLEAN] <dot_call>
    else if (accept_next(BOOLEAN, parser))
        status = parse_dot_call(parser);

    // <term> -> <string> <dot_call>
    else if (check_next_string_any(parser))
        status = parse_string(parser) && parse_dot_call(parser);

    // <term> -> <fun_call> <dot_call>
    else if (check_next(IDENTIFIER, parser))
        status = parse_function_call(parser) && parse_dot_call(parser);

    parser->debug_prefix_term = tmp_term_debug;
    if (status == FAILED)
        return failed(parser, PARSER_INVALID_EXPRESSION_USE,
            str_token(&parser->next, GET_NAME));
    return CORRECT;
}

int parse_string(ParserContext* parser)
{
    // <string> -> [STRING]
    // <string> -> [MULTILINE_STRING]
    if (accept_next(STRING, parser) || accept_next(MULTILINE_STRING, parser))
        return CORRECT;

    // <string> -> <interpolated_string>
    if (check_next(STRING_INTERPOLATION, parser)
        || check_next(STRING_INTERPOLATION_END, parser))
        return parse_interpolated_string(parser);

    // <string> -> <multiline_interpolated_string>
    if (check_next(MULTILINE_STRING_INTERPOLATION, parser)
        || check_next(MULTILINE_STRING_INTERPOLATION_END, parser))
        return parse_multiline_interpolated_string(parser);

    return failed(parser, PARSER_RULE_INCOMPLETE, str_token(&parser->next, GET_NAME));
}

int parse_interpolated_string(ParserContext* parser)
{
    // <interpolated_string> -> [STRING_INTERPOLATION_END]
    if (accept_next(STRING_INTERPOLATION_END, parser))
        return CORRECT;

    // <interpolated_string> -> [STRING_INTERPOLATION] <interpolated_string>
    if (accept_next(STRING_INTERPOLATION, parser))
        return parse_interpolated_string(parser);

    // <interpolated_string> -> <EXPR> <interpolated_string>
    const bool tmp = parser->interpolated_expression;
    parser->interpolated_expression = true;

    if (!parse_expression(parser, NULL))
        return FAILED;

    parser->interpolated_expression = tmp;
    return parse_interpolated_string(parser);
}

int parse_multiline_interpolated_string(ParserContext* parser)
{
    // <multiline_interpolated_string> -> [STRING_INTERPOLATION_END]
    if (accept_next(MULTILINE_STRING_INTERPOLATION_END, parser))
        return CORRECT;

    // <multiline_interpolated_string> -> [STRING_INTERPOLATION] <multiline_interpolated_string>
    if (accept_next(MULTILINE_STRING_INTERPOLATION, parser))
        return parse_multiline_interpolated_string(parser);

    // <multiline_interpolated_string> -> <EXPR> <multiline_interpolated_string>
    const bool tmp = parser->interpolated_expression;
    parser->interpolated_expression = true;

    if (!parse_expression(parser, NULL))
        return FAILED;

    parser->interpolated_expression = tmp;
    return parse_multiline_interpolated_string(parser);
}

int parse_dot_call(ParserContext* parser)
{
    if (accept_next(DOT, parser))
        return parse_function_call(parser) && parse_dot_call(parser);
    return CORRECT;
}

int parse_function_call(ParserContext* parser)
{
    return accept_next(IDENTIFIER, parser) && parse_function_call_group_list(parser);
}

int parse_function_call_group_list(ParserContext* parser)
{
    // <fun_call_group_list> -> ( <fun_call_param_first> ) <fun_call_group_list>
    if (accept_next(LEFT_BRACKET, parser)) {

        if (!parse_function_call_parameter_first(parser))
            return FAILED;

        if (!accept_next(RIGHT_BRACKET, parser))
            return FAILED;

        return parse_function_call_group_list(parser);
    }

    // <fun_call_group_list> -> 3
    return CORRECT;
}

int parse_function_call_parameter_first(ParserContext* parser)
{
    // <fun_call_param_first> -> 3
    if (check_next(RIGHT_BRACKET, parser))
        return CORRECT;

    // <fun_call_param_first> -> <EXPR> <fun_call_param_list>
    const bool tmp_parameter_expression = parser->parameter_expression;
    parser->parameter_expression = true;
    if (!parse_expression(parser, NULL))
        return FAILED;
    parser->parameter_expression = tmp_parameter_expression;

    return parse_function_call_parameter_list(parser);
}

int parse_function_call_parameter_list(ParserContext* parser)
{
    // <fun_call_param_list> -> , <EXPR> <fun_call_param_list>
    if (accept_next(COMMA, parser)) {

        parser->parameter_expression = true;
        if (!parse_expression(parser, NULL))
            return FAILED;
        parser->parameter_expression = false;

        return parse_function_call_parameter_list(parser);
    }

    // <fun_call_param_list> -> 3
    return CORRECT;
}

// =======================================================

int parse_expression(ParserContext* parser, ASTExpression** expr_target)
{
    static const char table[P_TERMINAL_COUNT][P_TERMINAL_COUNT] = {
        //                          [(]  [t]  [)]  [!]  [*]  [+]  [<]  [=]  [&]  [|]  [$]
        /* [(]              */ { '<', '<', '=', '<', '<', '<', '<', '<', '<', '<', '?' },
        /* [t]              */ { '?', '?', '>', '?', '>', '>', '>', '>', '>', '>', '>' },
        /* [)]              */ { '?', '?', '>', '?', '>', '>', '>', '>', '>', '>', '>' },
        /* [!, +u, -u]      */ { '<', '<', '>', '<', '>', '>', '>', '>', '>', '>', '>' },
        /* [*, /]           */ { '<', '<', '>', '<', '>', '>', '>', '>', '>', '>', '>' },
        /* [+, -]           */ { '<', '<', '>', '<', '<', '>', '>', '>', '>', '>', '>' },
        /* [<, <=, >, >=]   */ { '<', '<', '>', '<', '<', '<', '?', '>', '>', '>', '>' },
        /* [==, !=]         */ { '<', '<', '>', '<', '<', '<', '<', '?', '>', '>', '>' },
        /* [&&]             */ { '<', '<', '>', '<', '<', '<', '<', '<', '>', '>', '>' },
        /* [||]             */ { '<', '<', '>', '<', '<', '<', '<', '<', '<', '>', '>' },
        /* [$]              */ { '<', '<', '?', '<', '<', '<', '<', '<', '<', '<', '?' }
    };

    Stack* stack = init_stack();
    Stack* expression_stack = init_stack();
    parser->expressions_nested++;
    if (stack == NULL || expression_stack == NULL) {
        dispose_stack(stack);
        dispose_stack(expression_stack);
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence/expression stack");
    }

    while (true) {
        const PrecedenceElement last_elem = get_last_terminal_precedence(stack);
        const PrecedenceElement next_elem = get_next_token_precedence(parser, last_elem);

        if (precedence_stack_finished(stack, next_elem, parser)) {
            if (expr_target != NULL)
                *expr_target = top(expression_stack);
            dispose_stack(stack);
            dispose_stack(expression_stack);
            parser->expressions_nested--;
            return CORRECT;
        }

        switch (table[last_elem][next_elem]) {
            case '<':
                if (!stack_push_before_expression(stack, '<'))
                    return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");
                if (top_value(stack) != P_EXPRESSION && next_elem == P_ADDSUB) {
                    if (push_value(stack, P_UNARY) == 1)
                        return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");
                } else {
                    if (push_value(stack, next_elem) == 1)
                        return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");
                }
                break;
            case '=':
                if (push_value(stack, next_elem) == 1)
                    return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");
                break;
            case '>':
                if (!apply_expression_rule(stack, expression_stack, parser))
                    return FAILED;
                continue;
            default:
                return failed(parser, PARSER_INVALID_EXPRESSION_USE,
                    str_token(&parser->next, GET_ATTRIBUTE));
        }

        if (next_elem != P_TERM)
            next(parser);
        else if (parse_term(parser) == FAILED)
            return FAILED;

        if (parser->debug)
            log_precedence_stack(parser, stack);
    }

    return failed(parser, PARSER_RULE_INCOMPLETE, "expression");
}

// =======================================================

bool precedence_stack_finished(const Stack* stack, const int next_elem, const ParserContext* parser)
{
    if (top_value(stack) != P_EXPRESSION || stack_size(stack) != 1)
        return false;

    if (parser->parameter_expression && next_elem == P_R_BRACKET)
        return true;

    return (next_elem == P_DELIMITER);
}

int stack_push_before_expression(Stack* stack, const PrecedenceElement element)
{
    if (is_empty_stack(stack) || top_value(stack) != P_EXPRESSION)
        return push_value(stack, element) ? FAILED : CORRECT;

    pop(stack);

    if (push_value(stack, element) == 1)
        return FAILED;

    return push_value(stack, P_EXPRESSION) ? FAILED : CORRECT;
}

// =======================================================

int apply_expression_rule(Stack* stack, Stack* expression_stack, ParserContext* parser)
{
    if (parser->debug) {
        if (push_value(stack, '>') == 1)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");

        log_precedence_stack(parser, stack);
        pop(stack);
    }

    if (!stack_accept_rule(stack, expression_stack, parser))
        return failed(parser, PARSER_INVALID_EXPRESSION_FORMAT);

    if (!stack_accept_elem(stack, '<'))
        return failed(parser, PARSER_INVALID_EXPRESSION_FORMAT);

    if (push_value(stack, P_EXPRESSION) == 1)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");

    if (parser->debug)
        log_precedence_stack(parser, stack);

    return CORRECT;
}

int stack_accept_rule(Stack* stack, Stack* expression_stack, ParserContext* parser)
{
    ASTNode* node = ast_new_expression((ASTExpression) {
        .left = NULL,
        .right = NULL });

    // <E> -> [t]
    if (stack_accept_elem(stack, P_TERM)) {

        // TODO: node->expression.left = TERM
        if (push(expression_stack, node) == 1)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Expression");

        return CORRECT;
    }

    if (stack_accept_elem(stack, P_R_BRACKET)) {

        // <E> -> (<E>)
        if (stack_accept_elem(stack, P_EXPRESSION)) {
            return stack_accept_elem(stack, P_L_BRACKET);
        }

        // <E> -> ()
        else {
            // Unit literal
            return stack_accept_elem(stack, P_L_BRACKET);
        }
    }

    // Expression is required after this point
    if (!stack_accept_elem(stack, P_EXPRESSION))
        return FAILED;

    // <E> := +<E>, -<E>, !<E>
    if (stack_accept_elem(stack, P_UNARY)) {
        node->expression.left = top(expression_stack);

        pop(expression_stack);
        if (push(expression_stack, node) == 1)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Expression");

        return CORRECT;
    }

    // <E> :=
    // <E>*<E>, <E>/<E>, <E>+<E>, <E>-<E>,
    // <E><<E>, <E><=<E>, <E>><E>, <E>>=<E>,
    // <E>==<E>, <E>!=<E>, <E>&&<E>, <E>||<E>
    if (stack_accept_elem(stack, P_MULDIV)
        || stack_accept_elem(stack, P_ADDSUB)
        || stack_accept_elem(stack, P_COMPARE)
        || stack_accept_elem(stack, P_EQUAL)
        || stack_accept_elem(stack, P_AND)
        || stack_accept_elem(stack, P_OR)) {
        if (!stack_accept_elem(stack, P_EXPRESSION))
            return FAILED;

        node->expression.right = top(expression_stack);
        pop(expression_stack);
        node->expression.left = top(expression_stack);
        pop(expression_stack);

        if (push(expression_stack, node) == 1)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Expression");

        return CORRECT;
    }

    return FAILED;
}

int stack_accept_elem(Stack* stack, const int elem)
{
    if (top_value(stack) != elem)
        return FAILED;
    pop(stack);
    return CORRECT;
}

// =======================================================

PrecedenceElement get_last_terminal_precedence(Stack* stack)
{
    if (is_empty_stack(stack))
        return P_DELIMITER;

    if (top_value(stack) != P_EXPRESSION)
        return top_value(stack);

    const PrecedenceElement nonterminal = top_value(stack);
    pop(stack);

    const PrecedenceElement terminal = !is_empty_stack(stack) ? top_value(stack) : P_DELIMITER;
    push_value(stack, nonterminal);
    return terminal;
}

PrecedenceElement get_next_token_precedence(const ParserContext* parser, const int last_elem)
{
    if (parser->interpolated_expression) {
        if (check_next(STRING_INTERPOLATION_END, parser)
            || check_next(STRING_INTERPOLATION, parser)
            || check_next(MULTILINE_STRING_INTERPOLATION, parser)
            || check_next(MULTILINE_STRING_INTERPOLATION_END, parser))
            return P_DELIMITER;
    }

    if (parser->parameter_expression) {
        if (last_elem == P_R_BRACKET)
            return P_DELIMITER;
    }

    switch (parser->next.type) {
        case OPERATOR:
            switch (parser->next.attribute_operation_type) {
                case PLUS:
                case MINUS:
                    return P_ADDSUB;
                case MULTIPLY:
                case DIVIDE:
                    return P_MULDIV;
                case AND:
                    return P_AND;
                case OR:
                    return P_OR;
                case NOT:
                    return P_UNARY;
            }
            break;
        case COMPARE:
            if (parser->next.attribute_compare_type == EQUAL)
                return P_EQUAL;
            if (parser->next.attribute_compare_type == NOT_EQUAL)
                return P_EQUAL;
            return P_COMPARE;
        case LEFT_BRACKET:
            return P_L_BRACKET;
        case RIGHT_BRACKET:
            return P_R_BRACKET;
        case LINE_FEED:
        case SEMICOLON:
        case NONE:
        case ERROR:
        case COMMA:
        case END_OF_FILE:
        case KEYWORD:
            return P_DELIMITER;
        default:
            return P_TERM;
    }

    return P_TERM;
}

// =======================================================

void next(ParserContext* parser)
{
    if (get_line(parser) != parser->prev_line) {

        parser->prev_line = get_line(parser);

        if (parser->debug)
            print_line(parser);
    }

    parser->accepted = parser->next;

    if (parser->debug)
        log_parsed(parser);

    if (parser->future.type != NONE) {
        parser->next = parser->future;

        parser->future.type = NONE;
        parser->future_line = -1;
    } else {
        parser->next = get_token(&parser->scanner, stdin);
    }

    if (parser->next.type == ERROR)
        parser->token_error = parser->next.attribute_error;
}

// -------------------------------------------------------

bool check_future(const enum type_enum type, ParserContext* parser)
{
    parser->future_line = parser->scanner.line;
    parser->future = get_token(&parser->scanner, stdin);
    return parser->future.type == type;
}

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

bool check_next_string_any(const ParserContext* parser)
{
    if (check_next(STRING, parser))
        return true;
    if (check_next(STRING_INTERPOLATION, parser))
        return true;
    if (check_next(STRING_INTERPOLATION_END, parser))
        return true;
    if (check_next(MULTILINE_STRING, parser))
        return true;
    if (check_next(MULTILINE_STRING_INTERPOLATION, parser))
        return true;
    if (check_next(MULTILINE_STRING_INTERPOLATION_END, parser))
        return true;
    return false;
}

bool check_next_identifier(const char* name, const ParserContext* parser)
{
    if (!check_next(IDENTIFIER, parser))
        return false;

    return (strcmp(parser->next.attribute_string, name) == 0);
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
    if (check_next(LINE_FEED, parser))
        return parser->prev_line;

    if (parser->future_line != -1)
        return parser->future_line;

    return parser->scanner.line;
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
