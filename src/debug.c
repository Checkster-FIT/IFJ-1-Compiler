/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    debug.c
 * @brief   Various debug and logging functionality
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "debug.h"
#include "parser.h"
#include "scanner.h"
#include "stack.h"

void log_parsed(ParserContext* parser)
{
    if (parser->accepted.type == NONE)
        return;

    const char* prefix;
    const char* color = "\033[1;30m";

    // Accepted tokens (Recursive descent)
    if (parser->exit_code == 0) {
        prefix = "OK ";
        color = "\033[1;32m";
        // Skipped tokens
        if (parser->debug_prefix_skipped) {
            prefix = "----";
        }
        // Accepted tokens (Terms inside expressions)
        else if (parser->debug_prefix_term) {
            prefix = "TRM";
            color = "\033[1;34m";
        }
        // Accepted tokens (Expressions)
        else if (parser->debug_prefix_expression) {
            prefix = "EXP";
            color = "\033[1;34m";
        }
        // Accepted tokens (With AST related data)
        else if (parser->debug_prefix_ast) {
            prefix = "OK+";
            color = "\033[1;32m";
        }
    }
    // Invalid token
    else if (parser->debug_prefix_failed) {
        prefix = "ERR";
        color = "\033[1;31m";
        parser->debug_prefix_failed = false;
    }
    // Unprocessed tokens
    else if (parser->debug_print_remaining >= 0) {
        prefix = "   ";
    }
    // Hidden tokens
    else {
        return;
    }

    if (parser->debug_prefix_term && parser->debug_indent_terms)
        for (int i = 1; i < parser->expressions_nested; i++)
            fprintf(stderr, " \033[1;30m---\033[0m");

    fprintf(stderr, " %s%s\033[0m  ", color, prefix);
    print_token(&parser->accepted);

    parser->debug_prefix_skipped = false;
}

void log_remaining(ParserContext* parser)
{
    while (true) {
        next(parser);

        parser->debug_print_remaining--;

        if (parser->next.type == END_OF_FILE) {
            next(parser);
            break;
        }
    }

    if (parser->debug_print_remaining >= 0)
        return;

    fprintf(stderr, "[***] %i more remaining\n", -(parser->debug_print_remaining));
}

// =======================================================

void log_precedence_stack_reverse(const StackEl* current)
{
    if (current == NULL)
        return;

    log_precedence_stack_reverse(current->next);

    static const char* map[] = {
        [P_EXPRESSION] = " [E] ",
        [P_L_BRACKET] = "(",
        [P_TERM] = "t",
        [P_R_BRACKET] = ")",
        [P_UNARY] = "[UNARY]",
        [P_MULDIV] = "[MULDIV]",
        [P_ADDSUB] = "[ADDSUB]",
        [P_COMPARE] = "[CMP]",
        [P_EQUAL] = "[EQ]",
        [P_AND] = "[AND]",
        [P_OR] = "[OR]",
        [P_DELIMITER] = "$",
        [P_SPECIAL_START] = " <",
        [P_SPECIAL_END] = ">",
        [P_SPECIAL_EQUAL] = "=",
        [P_SPECIAL_INVALID] = "?",
    };

    const char* str = map_str(map, sizeof(map) / sizeof(map[0]), current->value, "?");
    fprintf(stderr, "%s", str);
}

void log_precedence_stack(const ParserContext* parser, const Stack* stack)
{
    if (!parser->debug_stack)
        return;

    if (parser->debug_indent_terms)
        for (int i = 1; i < parser->expressions_nested; i++)
            fprintf(stderr, "    ");

    fprintf(stderr, "      $");
    log_precedence_stack_reverse(stack->top);
    fprintf(stderr, "\n");
}

void print_line(ParserContext* parser)
{
    if (parser->exit_code != 0)
        return;

    const char* divider = "-----------------------";
    fprintf(stderr, "%s [line %i] %s\n", divider, get_line(parser), divider);
}

// =======================================================

void log_ast_start(ParserContext* parser)
{
    if (!parser->debug)
        return;

    parser->debug_prefix_ast = true;
}

void log_ast_end(ParserContext* parser)
{
    if (parser->debug == false)
        return;

    parser->debug_prefix_ast = false;

    if (!parser->debug_ast_nodes)
        return;

    const ASTNode* node = parser->ast->program.recent_node;
    static const char* format = "[\033[1;32m%s\033[0m] %-*s%s\n\n";

    const char* prefix = "AST";
    switch (node->type) {
        case AST_FUNCTION:
            fprintf(stderr, format, prefix, ALIGNMENT, "Function", node->function.identifier);
            break;
        case AST_VAR_DEF:
            fprintf(stderr, format, prefix, ALIGNMENT, "VarDef", node->vardef.identifier);
            break;
        case AST_PARAMETER:
            fprintf(stderr, format, prefix, ALIGNMENT, "Parameter", node->parameter.identifier);
            break;
        default:
            break;
    }
}

void print_ast(ASTNode* ast)
{
    // TODO: Clean up

    ASTNodeList* functions = &ast->program.functions;

    ASTNode* tmp_func = functions->active;
    functions->active = functions->first;

    fprintf(stderr, "\n");
    while (functions->active) {
        print_ast_function(functions->active, ast->program.global_symtable);
        functions->active = functions->active->next;
    }

    functions->active = tmp_func;
}

void print_ast_function(ASTNode* function_node, const Symtable* global_symtable)
{
    ASTFunction* function = &function_node->function;
    const SymtableEntry* entry = search(global_symtable, function->identifier)->value;

    fprintf(stderr, "     ");
    if (entry->main)
        fprintf(stderr, "@main ");

    fprintf(stderr, "%s( ", function->identifier);

    ASTNode* tmp_param = function->parameters.active;
    function->parameters.active = function->parameters.first;

    while (function->parameters.active) {
        print_ast_parameter(function->parameters.active, function->symtable);
        function->parameters.active = function->parameters.active->next;
    }

    function->parameters.active = tmp_param;

    fprintf(stderr, ") : %c", entry->data_type);

    fprintf(stderr, "\n");
}

void print_ast_parameter(const ASTNode* parameter_node, const Symtable* local_symtable)
{
    fprintf(stderr, "%s", parameter_node->parameter.identifier);

    const SymtableEntry* symtable_entry_param = (SymtableEntry*)search(local_symtable, parameter_node->parameter.identifier)->value;
    fprintf(stderr, ":%c ", symtable_entry_param->data_type);
}

// =======================================================

const char* str_token(const Token* token, const TokenStringType mode)
{
    static const char* name_map[] = {
        [NONE] = "NONE",
        [ERROR] = "ERROR",
        [INTEGER] = "INTEGER",
        [DOUBLE] = "DOUBLE",
        [OPERATOR] = "OPERATOR",
        [IDENTIFIER] = "IDENTIFIER",
        [MAIN_IDENTIFIER] = "MAIN_IDENTIFIER",
        [KEYWORD] = "KEYWORD",
        [BOOLEAN] = "BOOLEAN",
        [STRING_INTERPOLATION] = "STRING_INTERPOLATION",
        [STRING_INTERPOLATION_END] = "STRING_INTERPOLATION_END",
        [MULTILINE_STRING_INTERPOLATION] = "MULTILINE_STRING_INTERPOLATION",
        [MULTILINE_STRING_INTERPOLATION_END] = "MULTILINE_STRING_INTERPOLATION_END",
        [STRING] = "STRING",
        [MULTILINE_STRING] = "MULTILINE_STRING",
        [DOT] = "DOT",
        [DECLARATION_SIGN] = "DECLARATION_SIGN",
        [COMMA] = "COMMA",
        [SEMICOLON] = "SEMICOLON",
        [INDENT] = "INDENT",
        [DEDENT] = "DEDENT",
        [LINE_FEED] = "LINE_FEED",
        [WHITESPACE] = "WHITESPACE",
        [ASSIGNMENT] = "ASSIGNMENT",
        [COMPARE] = "COMPARE",
        [LEFT_BRACKET] = "LEFT_BRACKET",
        [RIGHT_BRACKET] = "RIGHT_BRACKET",
        [END_OF_FILE] = "END_OF_FILE"
    };

    static char num_to_str_buffer[BUFFER_SIZE];

    switch (token->type) {
        case INTEGER:
            snprintf(num_to_str_buffer, BUFFER_SIZE, "%li", token->attribute_int);
            break;
        case DOUBLE:
            snprintf(num_to_str_buffer, BUFFER_SIZE, "%lf", token->attribute_double);
            break;
        case ERROR:
            snprintf(num_to_str_buffer, BUFFER_SIZE, "%i", token->attribute_error);
            break;
        default:;
    }

    const char* attribute_map[] = {
        [DOT] = ".",
        [DECLARATION_SIGN] = ":",
        [SEMICOLON] = ";",
        [COMMA] = ",",
        [ASSIGNMENT] = "=",
        [LEFT_BRACKET] = "(",
        [RIGHT_BRACKET] = ")",
        [KEYWORD] = str_keyword(token->attribute_keyword),
        [OPERATOR] = str_operator(token->attribute_operation_type),
        [COMPARE] = str_compare(token->attribute_compare_type),
        [BOOLEAN] = (token->attribute_bool ? "true" : "false"),
        [IDENTIFIER] = token->attribute_string,
        [MAIN_IDENTIFIER] = token->attribute_string,
        [STRING] = token->attribute_string,
        [STRING_INTERPOLATION] = token->attribute_string,
        [STRING_INTERPOLATION_END] = token->attribute_string,
        [MULTILINE_STRING] = token->attribute_string,
        [MULTILINE_STRING_INTERPOLATION] = token->attribute_string,
        [MULTILINE_STRING_INTERPOLATION_END] = token->attribute_string,
        [INTEGER] = num_to_str_buffer,
        [DOUBLE] = num_to_str_buffer,
        [ERROR] = num_to_str_buffer
    };

    int size = 0;
    switch (mode) {
        case GET_NAME:
            size = sizeof(name_map) / sizeof(name_map[0]);
            return map_str(name_map, size, token->type, "[Unknown token]");
        case GET_ATTRIBUTE:
            size = sizeof(attribute_map) / sizeof(attribute_map[0]);
            return map_str(attribute_map, size, token->type, "");
        default:
            return "unknown";
    }
}

// =======================================================

void print_token(const Token* token)
{
    const char* name = str_token(token, GET_NAME);
    const char* attribute = str_token(token, GET_ATTRIBUTE);

    fprintf(stderr, "%-*s", ALIGNMENT, name);
    fprintf_escaped(stderr, attribute);
    fprintf(stderr, "\n");
}

// =======================================================

const char* str_keyword(const int idx)
{
    static const char* map[] = {
        [DEF_KW] = "def",
        [DO_KW] = "do",
        [DOUBLE_KW] = "Double",
        [ELSE_KW] = "else",
        [IF_KW] = "if",
        [IMPORT_KW] = "import",
        [INT_KW] = "Int",
        [RETURN_KW] = "return",
        [STRING_KW] = "String",
        [THEN_KW] = "then",
        [UNIT_KW] = "Unit",
        [VAL_KW] = "val",
        [VAR_KW] = "var",
        [WHILE_KW] = "while",
        [BOOLEAN_KW] = "Boolean",
        [CASE_KW] = "case",
        [CLASS_KW] = "class",
        [EXTENSION_KW] = "extension",
        [FOR_KW] = "for",
        [LIST_KW] = "List",
        [MATCH_KW] = "match",
        [TYPE_KW] = "type"
    };

    return map_str(map, sizeof(map) / sizeof(map[0]), idx, "[Unknown keyword]");
}

const char* str_operator(const int idx)
{
    static const char* map[] = {
        [PLUS] = "+",
        [MINUS] = "-",
        [MULTIPLY] = "*",
        [AND] = "&&",
        [OR] = "||",
        [NOT] = "!",
        [DIVIDE] = "/"
    };

    return map_str(map, sizeof(map) / sizeof(map[0]), idx, "[Unknown operator]");
}

const char* str_compare(const int idx)
{
    static const char* map[] = {
        [EQUAL] = "==",
        [NOT_EQUAL] = "!=",
        [LESS] = "<",
        [GREATER] = ">",
        [LESS_EQUAL] = "<=",
        [GREATER_EQUAL] = ">="
    };

    return map_str(map, sizeof(map) / sizeof(map[0]), idx, "[Unknown comparator]");
}

// =======================================================

void fprintf_escaped(FILE* stream, const char* string)
{
    if (string == NULL)
        return;

    for (size_t idx = 0; string[idx] != '\0'; idx++) {
        if (string[idx] == '\n')
            fprintf(stream, "\\n");
        else
            fputc(string[idx], stream);
    }
}

const char* map_str(const char** map, const int size, const int idx, const char* fallback)
{
    if (0 <= idx && idx < size && map[idx] != NULL)
        return map[idx];

    return fallback;
}
