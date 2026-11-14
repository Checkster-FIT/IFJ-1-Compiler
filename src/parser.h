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
#include "shared_strings.h"
#include "stack.h"
#include "symtable.h"
#include <stdbool.h>

#define CORRECT true
#define FAILED false

typedef struct {
    int exit_code;
    int token_error;
    Token future;
    Token next;
    Token accepted;

    ASTNode* ast;

    ScannerContext scanner;
    int prev_line;
    int future_line;
    Stack* symtable_stack;

    bool parameter_expression;
    bool interpolated_expression;
    int expressions_nested;
    char last_type;

    bool inside_extension;

    // Debug information
    bool debug;
    bool debug_stack;
    bool debug_indent_terms;
    bool debug_ast_nodes;
    bool debug_prefix_failed;
    bool debug_prefix_skipped;
    bool debug_prefix_term;
    bool debug_prefix_ast;
    int debug_print_remaining;

    // Testing information
    bool mockup_scanner;
    Token* mockup_tokens;
    size_t mockup_token_count;
    size_t mockup_token_index;

} ParserContext;

typedef enum {
    P_L_BRACKET,
    P_TERM,
    P_R_BRACKET,
    P_UNARY,
    P_MULDIV,
    P_ADDSUB,
    P_COMPARE,
    P_EQUAL,
    P_AND,
    P_OR,
    P_DELIMITER,
    P_DOT,
    P_TERMINAL_COUNT,
    P_NONE,
    P_EXPRESSION,
    P_SPECIAL_START = '<',
    P_SPECIAL_END = '>',
    P_SPECIAL_EQUAL = '=',
    P_SPECIAL_INVALID = '?'
} PrecedenceGroup;

typedef struct {
    PrecedenceGroup group;
    Operator operation;
} PrecedenceElement;

// -----------------------------------------
ParserContext* init_parser(ParserContext* parser, ASTNode* ast, AllocatedStrings* strings);

int run_parser(ASTNode* ast, AllocatedStrings* strings, ParserContext* external_context);

int destroy_parser(const ParserContext* parser);

Token retrieve_next_token(ParserContext* parser);

// -----------------------------------------
void next(ParserContext* parser);

bool check_future(const enum type_enum type, ParserContext* parser);

bool check_next(enum type_enum type, const ParserContext* parser);

bool check_next_keyword(enum attribute_keyword keyword, const ParserContext* parser);

bool check_next_identifier(const char* name, const ParserContext* parser);

bool check_next_operator(enum attribute_operation_type operation, const ParserContext* parser);

bool check_next_string_any(const ParserContext* parser);

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

int parse_separator(ParserContext* parser);

int parse_statement(ParserContext* parser);

int parse_statement_variable_definition(ParserContext* parser);

int parse_statement_assignment(ParserContext* parser);

int parse_statement_return(ParserContext* parser);

int parse_construct(ParserContext* parser);

int parse_construct_while(ParserContext* parser);

int parse_construct_if_then(ParserContext* parser);

int parse_construct_else(ParserContext* parser);

int parse_type_specifier(ParserContext* parser);

int parse_type(ParserContext* parser);

int parse_param_group_list(ParserContext* parser, SymtableEntry* entry);

int parse_param_list_first(ParserContext* parser, SymtableEntry* entry);

int parse_param_list(ParserContext* parser, SymtableEntry* entry);

int parse_param(ParserContext* parser, SymtableEntry* entry);

// -----------------------------------------
PrecedenceElement* create_next_precedence_terminal(const ParserContext* parser, PrecedenceElement last_elem);

PrecedenceElement get_last_precedence_terminal(Stack* stack);

int parse_expression(ParserContext* parser, ASTNode** expr_target);

int stack_push_before_expression(Stack* stack, const PrecedenceElement element);

bool precedence_stack_finished(const Stack* stack, int next_elem, const ParserContext* parser);

int apply_expression_rule(Stack* stack, ASTNode* term_node, Stack* expression_stack, ParserContext* parser);

int stack_accept_rule(Stack* stack, ASTNode* term_node, Stack* expression_stack, ParserContext* parser);

int stack_accept_elem(Stack* stack, const PrecedenceGroup group);

int get_stack_top_operator(const Stack* stack);

int parse_term(ParserContext* parser, ASTNode** term_target);

int parse_function_call(ParserContext* parser, ASTNode** term_target);

int parse_function_call_group_list(ParserContext* parser, ASTNode** term_target);

int parse_function_call_parameter_first(ParserContext* parser, ASTNode** term_target);

int parse_function_call_parameter_list(ParserContext* parser, ASTNode** term_target);

int parse_string(ParserContext* parser, ASTNode** term_target);

int parse_interpolated_string(ParserContext* parser, ASTNode* target_string_node);

int parse_multiline_interpolated_string(ParserContext* parser, ASTNode* target_string_node);

// -----------------------------------------

/**
 * Gets the correct line number adjusted for newlines
 * @param parser context with the line number and next token needed for the adjustment
 * @return the adjusted line number
 */
int get_line(const ParserContext* parser);

int failed(ParserContext* parser, int err, ...);

#endif // IFJ_1_COMPILER_PARSER_H