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

    parser->strings = strings,
    parser->ast = ast;
    push(parser->symtable_stack, ast->program.global_symtable);

    if (!add_built_in_entries(ast->program.global_symtable))
        return NULL;

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
    parser->last_type = '\0';
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

    // Disable unit testing scanner mockup
    parser->mockup_scanner = false;
    parser->mockup_tokens = NULL;
    parser->mockup_token_count = 0;
    parser->mockup_token_index = 0;

    parser->input_stream = stdin;
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

int run_parser(ASTNode* ast, AllocatedStrings* strings, ParserContext* external_context)
{
    ParserContext parser;

    if (external_context != NULL)
        parser = *external_context;
    else if (init_parser(&parser, ast, strings) == NULL)
        return print_error(MEMORY_ALLOCATION_ERROR, 0, "ParserContext");

    next(&parser);
    const int status = parse_program(&parser);

    if (parser.debug && status == FAILED)
        log_remaining(&parser);

    return destroy_parser(&parser);
}

// =======================================================

int parse_program(ParserContext* parser)
{
    // <program> -> [LF] <program> (Discard any leading line feeds)
    while (accept_next(LINE_FEED, parser)) { };
    // <program> -> [INDENT] <program>
    while (accept_next(INDENT, parser)) { };

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
    // <body> -> [DEDENT] <body>
    while (accept_next(DEDENT, parser)) { };

    // Multiple import headers
    if (check_next_keyword(IMPORT_KW, parser))
        return failed(parser, PARSER_MULTIPLE_IMPORT_HEADERS);

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
        return parse_function(parser, '\0') && parse_body(parser);

    if (check_next(DEDENT, parser))
        return CORRECT;

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
    ASTNode* extension_node = ast_add_extension(parser->ast, local_symtable);
    if (extension_node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST extension");
    extension_node->line = get_line(parser);
    log_ast_end(parser);

    if (!accept_next(LEFT_BRACKET, parser))
        return failed(parser, PARSER_MISSING_EXT_PARAM);

    ASTNode* group_node = ast_add_parameter_group(parser->ast, AST_TARGET_EXTENSION);
    group_node->line = get_line(parser);
    if (!parse_param(parser, NULL))
        return failed(parser, PARSER_MISSING_EXT_PARAM_END);
    char ext_type = parser->last_type;

    if (!accept_next(RIGHT_BRACKET, parser))
        return failed(parser, PARSER_MISSING_EXT_PARAM_END);

    if (!accept_next(LINE_FEED, parser))
        return failed(parser, PARSER_EXT_MISSING_NEWLINE);

    if (!accept_next(INDENT, parser))
        return failed(parser, PARSER_EXT_MISSING_INDENT);

    return parse_extension_body(parser, ext_type);
}

int parse_extension_body(ParserContext* parser, const char extension_type)
{
    // <extension_body> -> [DEDENT]
    if (accept_next(DEDENT, parser)) {
        pop(parser->symtable_stack);
        return CORRECT;
    }

    // <extension_body> -> <function> + <extension_body>
    if (check_next_keyword(DEF_KW, parser))
        return parse_function(parser, extension_type) && parse_extension_body(parser, extension_type);

    return failed(parser, PARSER_EXT_BLOCK_INVALID);
}

// -------------

int parse_function(ParserContext* parser, const char extension_type)
{
    log_ast_start(parser);
    // <function> -> def [IDENTIFIER] <param_group_list> <type_specifier> = <function_body>

    const bool is_main = accept_next(MAIN_IDENTIFIER, parser);

    if (is_main)
        accept_next(LINE_FEED, parser);

    if (!accept_next_keyword(DEF_KW, parser))
        return failed(parser, PARSER_INVALID_FUNCTION_DEF);

    if (!accept_next(IDENTIFIER, parser))
        return failed(parser, PARSER_INVALID_FUNCTION);

    // Identifier
    const char* identifier = parser->accepted.attribute_string;
    if (extension_type != '\0') {
        DynamicString* ext_identifier = init_dynamic_string();
        dynamic_string_append(ext_identifier, extension_type);
        dynamic_string_append(ext_identifier, '#');
        dynamic_string_append_str(ext_identifier, identifier);

        identifier = add_string_copy(parser->strings, ext_identifier->chars);
        dispose_dynamic_string(ext_identifier);
    }

    // Bypass extension symtable
    Symtable* global_symtable = parser->ast->program.global_symtable;
    SymtableEntry* function = symtable_add_entry(global_symtable,
        (SymtableEntry) {
            .defined = true,
            .function = true,
            .constant = true,
            .main = is_main,
            .data_type = '\0',
            .builtin = false,
            .extension = (extension_type != '\0'),
            .parameters = NULL,
            .identifier = identifier,
        });

    if (!function)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Symtable entry");
    if (function->error)
        return failed(parser, SEMANTIC_INVALID_REDEFINITION, function->identifier);

    function->parameters = init_dynamic_string();
    if (!function->parameters)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Function symtable parameters");

    // - Create a new local symtable
    Symtable* local_symtable = new_symtable();
    if (local_symtable == NULL || push(parser->symtable_stack, local_symtable))
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Local symtable");

    // AST - New function
    const ASTTarget target = parser->inside_extension ? AST_TARGET_EXTENSION : AST_TARGET_FUNCTION;
    ASTNode* fun_node = ast_add_function(parser->ast, function->identifier, local_symtable, target);
    if (fun_node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Function");
    fun_node->line = get_line(parser);
    log_ast_end(parser);

    const bool tmp = parser->inside_extension;
    parser->inside_extension = false;

    if (!parse_param_group_list(parser, function))
        return FAILED;

    parser->inside_extension = tmp;

    if (!parse_type_specifier(parser))
        return FAILED;

    function->data_type = parser->last_type;

    if (!accept_next(ASSIGNMENT, parser))
        return failed(parser, PARSER_MISSING_FUNCTION_ASSIGNMENT);

    if (!parse_function_body(parser))
        return FAILED;

    pop(parser->ast->program.block_stack);
    pop(parser->symtable_stack);
    return CORRECT;
}

int parse_type_specifier(ParserContext* parser)
{
    // <type_specifier> -> : <type>
    if (check_next(DECLARATION_SIGN, parser))
        return accept_next(DECLARATION_SIGN, parser) && parse_type(parser);

    // <type_specifier> -> 3
    parser->last_type = '\0';
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

int parse_param_group_list(ParserContext* parser, SymtableEntry* entry)
{
    // <param_group_list> -> ( <param_list_first> ) <param_group_list>
    if (check_next(LEFT_BRACKET, parser)) {
        if (!accept_next(LEFT_BRACKET, parser))
            return FAILED;

        if (entry && entry->parameters) {
            dynamic_string_append(entry->parameters, '#');
        }

        ASTNode* node = ast_add_parameter_group(parser->ast, AST_TARGET_FUNCTION);
        if (node == NULL)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Node");
        node->line = get_line(parser);

        if (!parse_param_list_first(parser, entry))
            return FAILED;

        if (!accept_next(RIGHT_BRACKET, parser))
            return failed(parser, PARSER_INVALID_PARAM_LIST_END);
        if (!parse_param_group_list(parser, entry))
            return FAILED;

        return CORRECT;
    }

    // <param_group_list> -> 3
    return CORRECT;
}

int parse_param_list_first(ParserContext* parser, SymtableEntry* entry)
{
    // Better error messages
    if (check_next(DECLARATION_SIGN, parser))
        return failed(parser, PARSER_MISSING_PARAM_IDENTIFIER);

    // <param_list_first> -> <param> <param_list>
    if (check_next(IDENTIFIER, parser))
        return parse_param(parser, entry) && parse_param_list(parser, entry);

    // <param_list_first> -> 3
    return CORRECT;
}

int parse_param_list(ParserContext* parser, SymtableEntry* entry)
{
    // <param_list> -> , <param> <param_list>
    if (check_next(COMMA, parser))
        return accept_next(COMMA, parser) && parse_param(parser, entry) && parse_param_list(parser, entry);

    // <param_list> -> 3
    return CORRECT;
}

int parse_param(ParserContext* parser, const SymtableEntry* entry)
{
    // Better error messages
    if (check_next(DECLARATION_SIGN, parser))
        return failed(parser, PARSER_MISSING_PARAM_IDENTIFIER);

    // <param> -> [IDENTIFIER] : <type>
    log_ast_start(parser);

    if (!accept_next(IDENTIFIER, parser))
        return FAILED;

    SymtableEntry parameter = {
        .defined = true,
        .function = false,
        .constant = true,
        .identifier = parser->accepted.attribute_string,
        .data_type = '\0',
    };

    if (!accept_next(DECLARATION_SIGN, parser))
        return failed(parser, PARSER_MISSING_PARAM_TYPE);

    if (!parse_type(parser))
        return FAILED;

    parameter.data_type = parser->last_type;
    if (entry && entry->parameters) {
        dynamic_string_append(entry->parameters, parser->last_type);
    }

    // AST - Parameter
    if (parser->inside_extension) {
        ASTNode* param = ast_add_parameter(parser->ast, parameter.identifier, AST_TARGET_EXTENSION);
        if (param == NULL)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Parameter");
        param->line = get_line(parser);
    } else {
        ASTNode* param = ast_add_parameter(parser->ast, parameter.identifier, AST_TARGET_FUNCTION);
        if (param == NULL)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Parameter");
        param->line = get_line(parser);
    }
    log_ast_end(parser);

    // Symtable - Parameter
    SymtableEntry* param_entry = symtable_add_entry(top(parser->symtable_stack), parameter);
    if (!param_entry)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Symtable entry");
    if (param_entry->error)
        return failed(parser, SEMANTIC_INVALID_REDEFINITION, param_entry->identifier);

    return CORRECT;
}

// -------------

int parse_function_body(ParserContext* parser)
{
    // <function_body> -> [LF] [INDENT] <statement_list> [DEDENT]
    if (check_next(LINE_FEED, parser)) {

        if (!accept_next(LINE_FEED, parser))
            return FAILED;

        if (!accept_next(INDENT, parser)) {

            // Better error message
            if (check_next_keyword(DEF_KW, parser) || check_next_keyword(EXTENSION_KW, parser)
                || check_next(DEDENT, parser)|| check_next(END_OF_FILE, parser))
                return failed(parser, PARSER_FUNCTION_MISSING_BODY);

            return failed(parser, PARSER_FUNCTION_MISSING_INDENT);
        }

        if (!parse_statement_list(parser))
            return FAILED;

        if (!accept_next(DEDENT, parser))
            return FAILED;

        return CORRECT;
    }

    // <function_body> -> <EXPR> [LF]
    ASTNode* expr_node;
    if (!parse_expression(parser, &expr_node))
        return FAILED;
    if (expr_node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Expression");
    expr_node->line = get_line(parser);
    ast_add_expression(parser->ast, expr_node);

    if (!parse_separator(parser))
        return failed(parser, PARSER_FUNCTION_SINGLE_LINE_MISSING_LINE_FEED);

    return CORRECT;
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
    ASTNode* expr_node = NULL;
    if (!parse_expression(parser, &expr_node))
        return FAILED;

    ASTNode* node = ast_add_expression(parser->ast, expr_node);
    if (node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Expression");
    node->line = get_line(parser);

    return CORRECT;
}

int parse_statement_variable_definition(ParserContext* parser)
{
    // <st_vardef> -> val [IDENTIFIER] <type_specifier> = <EXPR>
    // <st_vardef> -> var [IDENTIFIER] <type_specifier> = <EXPR>
    log_ast_start(parser);
    SymtableEntry vardef = {
        .defined = false,
        .function = false,
        .data_type = '\0',
    };

    if (!accept_next_keyword(VAR_KW, parser) && !accept_next_keyword(VAL_KW, parser))
        return FAILED;
    vardef.constant = parser->accepted.attribute_keyword == VAL_KW;

    if (!accept_next(IDENTIFIER, parser))
        return failed(parser, PARSER_ST_VAR_DEF_MISSING_IDENTIFIER);
    vardef.identifier = parser->accepted.attribute_string;

    if (!parse_type_specifier(parser))
        return FAILED;
    vardef.data_type = parser->last_type;

    SymtableEntry* vardef_entry = symtable_add_entry(top(parser->symtable_stack), vardef);
    if (!vardef_entry)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Symtable entry");
    if (vardef_entry->error)
        return failed(parser, SEMANTIC_INVALID_REDEFINITION, vardef_entry->identifier);

    if (!accept_next(ASSIGNMENT, parser))
        return failed(parser, PARSER_ST_VAR_DEF_MISSING_ASSIGNMENT);

    ASTNode* node = ast_add_vardef(parser->ast, vardef.identifier);
    if (node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST VarDef");
    node->line = get_line(parser);

    if (!parse_expression(parser, &(node->vardef.expression)))
        return FAILED;
    node->vardef.expression->parent = node;

    log_ast_end(parser);

    return CORRECT;
}

int parse_statement_assignment(ParserContext* parser)
{
    // <st_assign> -> [IDENTIFIER] = <EXPR>
    if (!accept_next(IDENTIFIER, parser))
        return FAILED;

    const char* identifier = parser->accepted.attribute_string;

    if (!accept_next(ASSIGNMENT, parser))
        return FAILED;

    ASTNode* node = ast_add_assignment(parser->ast, identifier);
    if (node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST VarDef");
    node->line = get_line(parser);

    if (!parse_expression(parser, &(node->assignment.expression)))
        return FAILED;
    node->assignment.expression->parent = node;

    return CORRECT;
}

int parse_statement_return(ParserContext* parser)
{
    // <st_return> -> return <EXPR>
    if (!accept_next_keyword(RETURN_KW, parser))
        return FAILED;

    ASTNode* node = ast_add_return(parser->ast);
    if (node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST VarDef");
    node->line = get_line(parser);

    node->return_st.expression = NULL;
    if (check_next(LINE_FEED, parser) || check_next(SEMICOLON, parser))
        return CORRECT;

    if (!parse_expression(parser, &(node->return_st.expression)))
        return FAILED;
    node->return_st.expression->parent = node;

    return CORRECT;
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

    Symtable* symtable = new_symtable();
    if (symtable == NULL || push(parser->symtable_stack, symtable))
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Local symtable");

    ASTNode* node = ast_add_while_do(parser->ast, symtable);
    if (node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST While do");
    node->line = get_line(parser);

    if (!parse_expression(parser, &(node->while_do.condition)))
        return FAILED;
    node->while_do.condition->parent = node;

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

    pop(parser->ast->program.block_stack);
    pop(parser->symtable_stack);
    return CORRECT;
}

int parse_construct_if_then(ParserContext* parser)
{
    // <c_if_then> -> if <REL_EXPR> then [LF*] [INDENT*] <statement_list> [DEDENT*] <c_else>

    if (!accept_next_keyword(IF_KW, parser))
        return FAILED;

    ASTNode* if_node = ast_add_if(parser->ast);
    if (if_node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST If Node");
    if (!parse_expression(parser, &if_node->if_statement.condition))
        return FAILED;
    if_node->if_statement.condition->parent = if_node;
    if_node->line = get_line(parser);

    if (!accept_next_keyword(THEN_KW, parser))
        return failed(parser, PARSER_IF_MISSING_THEN);

    const bool indented = accept_next(LINE_FEED, parser);

    if (indented && !accept_next(INDENT, parser))
        return failed(parser, PARSER_IF_MISSING_INDENT);

    Symtable* then_symtable = new_symtable();
    if (then_symtable == NULL || push(parser->symtable_stack, then_symtable))
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Local symtable");

    ASTNode* then_node = ast_add_if_branch(parser->ast, AST_THEN, if_node, then_symtable);
    if (then_node == NULL)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Then Node");
    then_node->line = get_line(parser);
    if (!parse_statement_list(parser))
        return FAILED;
    pop(parser->ast->program.block_stack);
    pop(parser->symtable_stack);

    if (indented && !accept_next(DEDENT, parser))
        return FAILED;

    if (check_next_keyword(ELSE_KW, parser)) {

        Symtable* else_symtable = new_symtable();
        if (else_symtable == NULL || push(parser->symtable_stack, else_symtable))
            return failed(parser, MEMORY_ALLOCATION_ERROR, "Local symtable");

        ASTNode* else_node = ast_add_if_branch(parser->ast, AST_ELSE, if_node, else_symtable);
        if (else_node == NULL)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Else Node");
        else_node->line = get_line(parser);
        if (!parse_construct_else(parser))
            return FAILED;

        pop(parser->ast->program.block_stack);
        pop(parser->symtable_stack);
        return CORRECT;
    }

    return CORRECT;
}

int parse_construct_else(ParserContext* parser)
{
    // <c_else> -> else [LF*] [INDENT*] <statement_list> [DEDENT*]

    if (!accept_next_keyword(ELSE_KW, parser))
        return FAILED;

    // Else-if
    if (check_next_keyword(IF_KW, parser))
        return parse_construct_if_then(parser);

    const bool indented = accept_next(LINE_FEED, parser);

    if (indented && !accept_next(INDENT, parser))
        return failed(parser, PARSER_IF_MISSING_INDENT);

    if (!parse_statement_list(parser))
        return FAILED;

    if (indented && !accept_next(DEDENT, parser))
        return FAILED;

    return CORRECT;
}

int parse_term(ParserContext* parser, ASTNode** term_target)
{
    int status = FAILED;

    const bool tmp_term_debug = parser->debug_prefix_term;
    parser->debug_prefix_term = true;

    // <term> -> [INTEGER] <dot_call>
    if (accept_next(INTEGER, parser)) {
        *term_target = ast_new_literal((ASTLiteral) {
            .type = AST_INTEGER,
            .val_integer = parser->accepted.attribute_int,
        });
        (*term_target)->line = get_line(parser);
        status = CORRECT;
    }

    // <term> -> [DOUBLE] <dot_call>
    else if (accept_next(DOUBLE, parser)) {
        *term_target = ast_new_literal((ASTLiteral) {
            .type = AST_DOUBLE,
            .val_double = parser->accepted.attribute_double,
        });
        (*term_target)->line = get_line(parser);
        status = CORRECT;
    }

    // <term> -> [BOOLEAN] <dot_call>
    else if (accept_next(BOOLEAN, parser)) {
        *term_target = ast_new_literal((ASTLiteral) {
            .type = AST_BOOLEAN,
            .val_bool = parser->accepted.attribute_bool,
        });
        (*term_target)->line = get_line(parser);
        status = CORRECT;
    }

    // <term> -> <string> <dot_call>
    else if (check_next_string_any(parser)) {
        status = parse_string(parser, term_target);
    }

    // <term> -> <fun_call> <dot_call>
    else if (check_next(IDENTIFIER, parser)) {
        *term_target = ast_new_term(parser->next.attribute_string);
        (*term_target)->line = get_line(parser);
        status = parse_function_call(parser, term_target);
    }

    parser->debug_prefix_term = tmp_term_debug;
    if (status == FAILED)
        return failed(parser, PARSER_INVALID_EXPRESSION_USE,
            str_token(&parser->next, GET_NAME));

    return CORRECT;
}

int parse_string(ParserContext* parser, ASTNode** term_target)
{
    // <string> -> [STRING]
    // <string> -> [MULTILINE_STRING]
    if (accept_next(STRING, parser) || accept_next(MULTILINE_STRING, parser)) {
        *term_target = ast_new_literal((ASTLiteral) {
            .type = AST_STRING,
            .val_string = parser->accepted.attribute_string,
        });
        (*term_target)->line = get_line(parser);
        return CORRECT;
    }

    // <string> -> <interpolated_string>
    if (check_next(STRING_INTERPOLATION, parser)
        || check_next(STRING_INTERPOLATION_END, parser)) {

        *term_target = ast_new_interpolated_string();
        (*term_target)->line = get_line(parser);
        return parse_interpolated_string(parser, *term_target);
    }

    // <string> -> <multiline_interpolated_string>
    if (check_next(MULTILINE_STRING_INTERPOLATION, parser)
        || check_next(MULTILINE_STRING_INTERPOLATION_END, parser)) {

        *term_target = ast_new_interpolated_string();
        (*term_target)->line = get_line(parser);
        return parse_multiline_interpolated_string(parser, *term_target);
    }

    return failed(parser, PARSER_RULE_INCOMPLETE, str_token(&parser->next, GET_NAME));
}

int parse_interpolated_string(ParserContext* parser, ASTNode* target_string_node)
{
    // <interpolated_string> -> [STRING_INTERPOLATION_END]
    if (accept_next(STRING_INTERPOLATION_END, parser)) {
        ASTNode* string = ast_new_literal((ASTLiteral) {
            .type = AST_STRING,
            .val_string = parser->accepted.attribute_string,
        });
        string->line = get_line(parser);
        string->parent = target_string_node;
        ast_list_add(&target_string_node->interpolated_string.expressions, string);
        return CORRECT;
    }

    // <interpolated_string> -> [STRING_INTERPOLATION] <interpolated_string>
    if (accept_next(STRING_INTERPOLATION, parser)) {

        // Empty interpolated string that are not END can be discarded
        if (*parser->accepted.attribute_string == '\0')
            return parse_interpolated_string(parser, target_string_node);

        ASTNode* string = ast_new_literal((ASTLiteral) {
            .type = AST_STRING,
            .val_string = parser->accepted.attribute_string,
        });
        string->line = get_line(parser);
        string->parent = target_string_node;
        ast_list_add(&target_string_node->interpolated_string.expressions, string);
        return parse_interpolated_string(parser, target_string_node);
    }

    // <interpolated_string> -> <EXPR> <interpolated_string>
    const bool tmp = parser->interpolated_expression;
    parser->interpolated_expression = true;

    ASTNode* expression_result;
    if (!parse_expression(parser, &expression_result))
        return FAILED;
    expression_result->line = get_line(parser);
    expression_result->parent = target_string_node;
    ast_list_add(&target_string_node->interpolated_string.expressions, expression_result);

    parser->interpolated_expression = tmp;
    return parse_interpolated_string(parser, target_string_node);
}

int parse_multiline_interpolated_string(ParserContext* parser, ASTNode* target_string_node)
{
    // <multiline_interpolated_string> -> [STRING_INTERPOLATION_END]
    if (accept_next(MULTILINE_STRING_INTERPOLATION_END, parser)) {
        ASTNode* string = ast_new_literal((ASTLiteral) {
            .type = AST_STRING,
            .val_string = parser->accepted.attribute_string,
        });
        string->line = get_line(parser);
        string->parent = target_string_node;
        ast_list_add(&target_string_node->interpolated_string.expressions, string);
        return CORRECT;
    }

    // <multiline_interpolated_string> -> [STRING_INTERPOLATION] <multiline_interpolated_string>
    const int tmp_line = get_line(parser);
    if (accept_next(MULTILINE_STRING_INTERPOLATION, parser)) {

        // Empty interpolated string that are not END can be discarded
        if (*parser->accepted.attribute_string == '\0')
            return parse_interpolated_string(parser, target_string_node);

        ASTNode* string = ast_new_literal((ASTLiteral) {
            .type = AST_STRING,
            .val_string = parser->accepted.attribute_string,
        });
        string->line = tmp_line;
        string->parent = target_string_node;
        ast_list_add(&target_string_node->interpolated_string.expressions, string);
        return parse_multiline_interpolated_string(parser, target_string_node);
    }

    // <multiline_interpolated_string> -> <EXPR> <multiline_interpolated_string>
    const bool tmp = parser->interpolated_expression;
    parser->interpolated_expression = true;

    ASTNode* expression_result;
    if (!parse_expression(parser, &expression_result))
        return FAILED;
    expression_result->line = get_line(parser);
    expression_result->parent = target_string_node;
    ast_list_add(&target_string_node->interpolated_string.expressions, expression_result);

    parser->interpolated_expression = tmp;
    return parse_multiline_interpolated_string(parser, target_string_node);
}

int parse_function_call(ParserContext* parser, ASTNode** term_target)
{
    return accept_next(IDENTIFIER, parser) && parse_function_call_group_list(parser, term_target);
}

int parse_function_call_group_list(ParserContext* parser, ASTNode** term_target)
{
    // <fun_call_group_list> -> ( <fun_call_param_first> ) <fun_call_group_list>
    if (accept_next(LEFT_BRACKET, parser)) {

        ASTNode* arg_group = ast_add_argument_group(parser->ast, *term_target);
        if (arg_group == NULL)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Node");
        arg_group->line = get_line(parser);

        if (!parse_function_call_parameter_first(parser, &arg_group))
            return FAILED;

        if (!accept_next(RIGHT_BRACKET, parser))
            return FAILED;

        return parse_function_call_group_list(parser, term_target);
    }

    // <fun_call_group_list> -> 3
    return CORRECT;
}

int parse_function_call_parameter_first(ParserContext* parser, ASTNode** term_target)
{
    // <fun_call_param_first> -> 3
    if (check_next(RIGHT_BRACKET, parser))
        return CORRECT;

    // <fun_call_param_first> -> <EXPR> <fun_call_param_list>
    const bool tmp_parameter_expression = parser->parameter_expression;
    parser->parameter_expression = true;

    ASTNode* expression_result;
    if (!parse_expression(parser, &expression_result))
        return FAILED;
    expression_result->parent = *term_target;
    ast_list_add(&(*term_target)->argument_group.arguments, expression_result);

    parser->parameter_expression = tmp_parameter_expression;

    return parse_function_call_parameter_list(parser, term_target);
}

int parse_function_call_parameter_list(ParserContext* parser, ASTNode** term_target)
{
    // <fun_call_param_list> -> , <EXPR> <fun_call_param_list>
    if (accept_next(COMMA, parser)) {

        const bool tmp_parameter_expression = parser->parameter_expression;
        parser->parameter_expression = true;

        ASTNode* expression_result;
        if (!parse_expression(parser, &expression_result))
            return FAILED;
        expression_result->parent = *term_target;
        ast_list_add(&(*term_target)->argument_group.arguments, expression_result);

        parser->parameter_expression = tmp_parameter_expression;

        return parse_function_call_parameter_list(parser, term_target);
    }

    // <fun_call_param_list> -> 3
    return CORRECT;
}

// =======================================================

int parse_expression(ParserContext* parser, ASTNode** expr_target)
{
    static const char table[P_TERMINAL_COUNT][P_TERMINAL_COUNT] = {
        //                          [(]  [t]  [)]  [!]  [*]  [+]  [<]  [=]  [&]  [|]  [$]  [.]
        /* [(]              */ { '<', '<', '=', '<', '<', '<', '<', '<', '<', '<', '?', '<' },
        /* [t]              */ { '?', '?', '>', '?', '>', '>', '>', '>', '>', '>', '>', '>' },
        /* [)]              */ { '?', '?', '>', '?', '>', '>', '>', '>', '>', '>', '>', '>' },
        /* [!, +u, -u]      */ { '<', '<', '>', '<', '>', '>', '>', '>', '>', '>', '>', '<' },
        /* [*, /]           */ { '<', '<', '>', '<', '>', '>', '>', '>', '>', '>', '>', '<' },
        /* [+, -]           */ { '<', '<', '>', '<', '<', '>', '>', '>', '>', '>', '>', '<' },
        /* [<, <=, >, >=]   */ { '<', '<', '>', '<', '<', '<', '?', '>', '>', '>', '>', '<' },
        /* [==, !=]         */ { '<', '<', '>', '<', '<', '<', '<', '?', '>', '>', '>', '<' },
        /* [&&]             */ { '<', '<', '>', '<', '<', '<', '<', '<', '>', '>', '>', '<' },
        /* [||]             */ { '<', '<', '>', '<', '<', '<', '<', '<', '<', '>', '>', '<' },
        /* [$]              */ { '<', '<', '?', '<', '<', '<', '<', '<', '<', '<', '?', '<' },
        /* [.]              */ { '?', '<', '>', '?', '>', '>', '>', '>', '>', '>', '>', '>' }
    };

    ASTNode* term_node = NULL;

    Stack* stack = init_stack();
    Stack* expression_stack = init_stack();
    parser->expressions_nested++;
    if (stack == NULL || expression_stack == NULL) {
        dispose_stack(stack);
        dispose_stack(expression_stack);
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence/expression stack");
    }

    while (true) {
        const PrecedenceElement last_elem = get_last_precedence_terminal(stack);
        PrecedenceElement* next_elem = create_next_precedence_terminal(parser, last_elem);
        PrecedenceElement* top_elem = top(stack);

        if (precedence_stack_finished(stack, next_elem->group, parser)) {
            if (expr_target != NULL)
                *expr_target = top(expression_stack);
            else
                free_ast_nodes(top(expression_stack));

            free(top(stack));
            free(next_elem);
            dispose_stack(stack);
            dispose_stack(expression_stack);
            parser->expressions_nested--;
            return CORRECT;
        }

        switch (table[last_elem.group][next_elem->group]) {
            case '<':
                if (!stack_push_before_expression(stack, (PrecedenceElement) { .group = '<' })) {
                    free(next_elem);
                    return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");
                }

                if ((!top_elem || top_elem->group != P_EXPRESSION) && next_elem->group == P_ADDSUB) {
                    next_elem->group = P_UNARY;
                    next_elem->operation = next_elem->operation == OP_ADD ? OP_UNARY_PLUS : next_elem->operation;
                    next_elem->operation = next_elem->operation == OP_SUB ? OP_UNARY_MINUS : next_elem->operation;
                }

                if (push(stack, next_elem) == 1) {
                    free(next_elem);
                    return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");
                }
                break;
            case '=':
                if (push(stack, next_elem) == 1) {
                    free(next_elem);
                    return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");
                }
                break;
            case '>':
                if (!apply_expression_rule(stack, term_node, expression_stack, parser)) {
                    free(next_elem);
                    return FAILED;
                }
                free(next_elem);
                continue;
            default:
                while (top(expression_stack)) {
                    free_ast_nodes(top(expression_stack));
                    pop(expression_stack);
                }
                while (top(stack)) {
                    free(top(stack));
                    pop(stack);
                }
                if (last_elem.group == P_TERM && term_node != NULL)
                    free_ast_nodes(term_node);
                free(next_elem);
                dispose_stack(stack);
                dispose_stack(expression_stack);

                if (parser->next.type != ERROR) {
                    return failed(parser, PARSER_INVALID_EXPRESSION_USE,
                        str_token(&parser->next, GET_ATTRIBUTE));
                }
                return FAILED;
        }

        if (next_elem->group != P_TERM)
            next(parser);
        else if (parse_term(parser, &term_node) == FAILED)
            return FAILED;

        if (parser->debug)
            log_precedence_stack(parser, stack);
    }

    return failed(parser, PARSER_RULE_INCOMPLETE, "expression");
}

// =======================================================

bool precedence_stack_finished(const Stack* stack, const int next_elem, const ParserContext* parser)
{
    const PrecedenceElement* top_elem = top(stack);

    if (top_elem == NULL)
        return false;

    if (top_elem->group != P_EXPRESSION || stack_size(stack) != 1)
        return false;

    if (parser->parameter_expression && next_elem == P_R_BRACKET)
        return true;

    return (next_elem == P_DELIMITER);
}

int stack_push_before_expression(Stack* stack, const PrecedenceElement element)
{
    PrecedenceElement* node = malloc(sizeof(PrecedenceElement));
    if (!node)
        return FAILED;

    *node = element;

    const PrecedenceElement* top_elem = top(stack);
    if (is_empty_stack(stack) || top_elem->group != P_EXPRESSION)
        return push(stack, node) ? FAILED : CORRECT;

    void* tmp = top(stack);
    pop(stack);

    if (push(stack, node) == 1)
        return FAILED;

    return push(stack, tmp) ? FAILED : CORRECT;
}

// =======================================================

int apply_expression_rule(Stack* stack, ASTNode* term_node, Stack* expression_stack, ParserContext* parser)
{
    if (parser->debug) {

        PrecedenceElement* node = malloc(sizeof(PrecedenceElement));
        if (!node)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");

        node->group = '>';
        node->operation = OP_NONE;
        if (push(stack, node) == 1)
            return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");

        log_precedence_stack(parser, stack);
        free(top(stack));
        pop(stack);
    }

    if (!stack_accept_rule(stack, term_node, expression_stack, parser))
        return failed(parser, PARSER_INVALID_EXPRESSION_FORMAT);

    if (!stack_accept_elem(stack, '<'))
        return failed(parser, PARSER_INVALID_EXPRESSION_FORMAT);

    PrecedenceElement* expr_element = malloc(sizeof(PrecedenceElement));
    expr_element->group = P_EXPRESSION;
    expr_element->operation = OP_NONE;

    if (!expr_element)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");

    if (push(stack, expr_element) == 1)
        return failed(parser, MEMORY_ALLOCATION_ERROR, "Precedence stack");

    if (parser->debug)
        log_precedence_stack(parser, stack);

    return CORRECT;
}

int stack_accept_rule(Stack* stack, ASTNode* term_node, Stack* expression_stack, ParserContext* parser)
{
    // <E> -> [t]
    if (stack_accept_elem(stack, P_TERM)) {

        if (push(expression_stack, term_node) == 1) {
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Expression");
        }

        return CORRECT;
    }

    ASTNode* node = ast_new_expression((ASTExpression) {
        .left = NULL,
        .right = NULL,
        .operation = OP_NONE,
    });
    node->line = get_line(parser);

    if (stack_accept_elem(stack, P_R_BRACKET)) {

        // <E> -> (<E>)
        if (stack_accept_elem(stack, P_EXPRESSION)) {
            free(node);
            return stack_accept_elem(stack, P_L_BRACKET);
        }

        // <E> -> ()
        else {
            ASTNode* unit_literal = ast_new_literal((ASTLiteral) { .type = AST_UNIT });

            if (!unit_literal || push(expression_stack, unit_literal) == 1) {
                return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Expression");
            }

            unit_literal->line = get_line(parser);
            free(node);
            return stack_accept_elem(stack, P_L_BRACKET);
        }
    }

    // Expression is required after this point
    if (!stack_accept_elem(stack, P_EXPRESSION)) {
        free(node);
        return FAILED;
    }

    node->expression.operation = get_stack_top_operator(stack);

    // <E> := +<E>, -<E>, !<E>
    if (stack_accept_elem(stack, P_UNARY)) {
        node->expression.left = top(expression_stack);
        node->expression.left->parent = node;
        pop(expression_stack);

        if (push(expression_stack, node) == 1) {
            free(node);
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Expression");
        }

        return CORRECT;
    }

    // <E> :=
    // <E>.<E>
    // <E>*<E>, <E>/<E>, <E>+<E>, <E>-<E>,
    // <E><<E>, <E><=<E>, <E>><E>, <E>>=<E>,
    // <E>==<E>, <E>!=<E>, <E>&&<E>, <E>||<E>
    if (stack_accept_elem(stack, P_MULDIV)
        || stack_accept_elem(stack, P_ADDSUB)
        || stack_accept_elem(stack, P_COMPARE)
        || stack_accept_elem(stack, P_EQUAL)
        || stack_accept_elem(stack, P_AND)
        || stack_accept_elem(stack, P_OR)
        || stack_accept_elem(stack, P_DOT)) {
        if (!stack_accept_elem(stack, P_EXPRESSION)) {
            free(node);
            return FAILED;
        }

        node->expression.right = top(expression_stack);
        node->expression.right->parent = node;
        pop(expression_stack);
        node->expression.left = top(expression_stack);
        node->expression.left->parent = node;
        pop(expression_stack);

        if (push(expression_stack, node) == 1) {
            free(node);
            return failed(parser, MEMORY_ALLOCATION_ERROR, "AST Expression");
        }

        return CORRECT;
    }

    free(node);
    return FAILED;
}

int get_stack_top_operator(const Stack* stack)
{
    PrecedenceElement* top_elem = top(stack);
    if (!top_elem)
        return OP_NONE;

    return top_elem->operation;
}

int stack_accept_elem(Stack* stack, const PrecedenceGroup group)
{
    if (is_empty_stack(stack))
        return FAILED;

    const PrecedenceElement* top_elem = top(stack);
    if (top_elem->group != group)
        return FAILED;

    free(top(stack));
    pop(stack);
    return CORRECT;
}

// =======================================================

PrecedenceElement get_last_precedence_terminal(Stack* stack)
{
    if (is_empty_stack(stack))
        return (PrecedenceElement) {
            .group = P_DELIMITER,
            .operation = OP_NONE
        };

    PrecedenceElement* top_elem = top(stack);
    if (top_elem->group != P_EXPRESSION)
        return *top_elem;

    pop(stack);

    if (is_empty_stack(stack)) {
        push(stack, top_elem);
        return (PrecedenceElement) {
            .group = P_DELIMITER,
            .operation = OP_NONE
        };
    }

    const PrecedenceElement* prev_elem = top(stack);
    push(stack, top_elem);
    return *prev_elem;
}

PrecedenceElement* create_next_precedence_terminal(const ParserContext* parser, PrecedenceElement last_elem)
{
    PrecedenceElement* element = malloc(sizeof(PrecedenceElement));
    if (!element)
        return NULL;
    element->operation = OP_NONE;

    if (parser->interpolated_expression) {
        if (check_next(STRING_INTERPOLATION_END, parser)
            || check_next(STRING_INTERPOLATION, parser)
            || check_next(MULTILINE_STRING_INTERPOLATION, parser)
            || check_next(MULTILINE_STRING_INTERPOLATION_END, parser)) {

            element->group = P_DELIMITER;
            return element;
        }
    }

    if (parser->parameter_expression) {
        if (last_elem.group == P_R_BRACKET) {
            element->group = P_DELIMITER;
            return element;
        }
    }

    switch (parser->next.type) {
        case OPERATOR:
            switch (parser->next.attribute_operation_type) {
                case PLUS:
                    element->group = P_ADDSUB;
                    element->operation = OP_ADD;
                    return element;
                case MINUS:
                    element->group = P_ADDSUB;
                    element->operation = OP_SUB;
                    return element;
                case MULTIPLY:
                    element->group = P_MULDIV;
                    element->operation = OP_MUL;
                    return element;
                case DIVIDE:
                    element->group = P_MULDIV;
                    element->operation = OP_DIV;
                    return element;
                case AND:
                    element->group = P_AND;
                    element->operation = OP_AND;
                    return element;
                case OR:
                    element->group = P_OR;
                    element->operation = OP_OR;
                    return element;
                case NOT:
                    element->group = P_UNARY;
                    element->operation = OP_NOT;
                    return element;
            }
            break;
        case COMPARE:
            element->group = P_COMPARE;
            switch (parser->next.attribute_compare_type) {
                case EQUAL:
                    element->group = P_EQUAL;
                    element->operation = OP_EQUAL;
                    return element;
                case NOT_EQUAL:
                    element->group = P_EQUAL;
                    element->operation = OP_NOT_EQUAL;
                    return element;
                case LESS:
                    element->operation = OP_LESS;
                    return element;
                case GREATER:
                    element->operation = OP_GREATER;
                    return element;
                case LESS_EQUAL:
                    element->operation = OP_LESS_EQUAL;
                    return element;
                case GREATER_EQUAL:
                    element->operation = OP_GREATER_EQUAL;
                    return element;
            }
            break;
        case LEFT_BRACKET:
            element->group = P_L_BRACKET;
            return element;
        case RIGHT_BRACKET:
            element->group = P_R_BRACKET;
            return element;
        case DOT:
            element->group = P_DOT;
            element->operation = OP_DOT;
            return element;
        case LINE_FEED:
        case SEMICOLON:
        case NONE:
        case ERROR:
        case COMMA:
        case END_OF_FILE:
        case KEYWORD:
            element->group = P_DELIMITER;
            return element;
        default:
            element->group = P_TERM;
            element->operation = OP_NONE;
            return element;
    }
    return element;
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
        parser->next = retrieve_next_token(parser);
    }

    if (parser->next.type == ERROR)
        parser->token_error = parser->next.attribute_error;
}

// -------------------------------------------------------

Token retrieve_next_token(ParserContext* parser)
{
    if (parser->mockup_scanner) {

        if (parser->mockup_token_index >= parser->mockup_token_count)
            return (Token) { .type = END_OF_FILE };

        const Token token = parser->mockup_tokens[parser->mockup_token_index];
        parser->mockup_token_index++;
        return token;
    }

    return get_token(&parser->scanner, parser->input_stream);
}

// -------------------------------------------------------

bool check_future(const enum type_enum type, ParserContext* parser)
{
    parser->future_line = parser->scanner.line;
    parser->future = retrieve_next_token(parser);
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

    if (check_next(MULTILINE_STRING, parser)
        || check_next(MULTILINE_STRING_INTERPOLATION, parser)
        || check_next(MULTILINE_STRING_INTERPOLATION_END, parser)) {

        if (parser->scanner.line >= parser->prev_line)
            return parser->prev_line;
    }

    if (parser->future_line != -1)
        return parser->future_line;

    return parser->scanner.line;
}

int failed(ParserContext* parser, const int err, ...)
{
    if (parser->token_error != 0)
        return FAILED;

    va_list args = { };
    va_start(args, err);

    parser->exit_code = va_print_error(err, get_line(parser), args);

    va_end(args);
    return FAILED;
}

// =======================================================
