/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    debug.c
 * @brief   Various debug and logging functionality
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "debug.h"
#include "parser.h"
#include "scanner.h"
#include "shared_data.h"

void log_parsed(ParserContext* parser)
{
    if (parser->accepted.type == NONE)
        return;

    const char* prefix;
    const char* color = "\033[1;30m";

    // Skipped tokens
    if (parser->debug_prefix_skipped) {
        prefix = "--";
    }
    // Accepted tokens (Expressions)
    else if (parser->debug_prefix_expression) {
        prefix = "OK";
        color = "\033[1;34m";
    }
    // Accepted tokens (Recursive descent)
    else if (parser->exit_code == 0) {
        prefix = "OK";
        color = "\033[1;32m";
    }
    // Invalid token
    else if (parser->debug_prefix_failed) {
        prefix = "!?";
        color = "\033[1;31m";
        parser->debug_prefix_failed = false;
    }
    // Unprocessed tokens
    else if (parser->debug_print_remaining >= 0) {
        prefix = "  ";
    }
    // Hidden tokens
    else {
        return;
    }

    fprintf(stderr, "[%s%s\033[0m] ", color, prefix);
    print_token(&parser->accepted, parser->shared_data);

    parser->debug_prefix_skipped = false;
    parser->debug_prefix_expression = false;
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

    fprintf(stderr, "[**] %i more remaining\n", -(parser->debug_print_remaining));
}

void log_ast_added(const ParserContext* parser)
{
    if (parser->debug == false)
        return;

    const ASTNode* node = parser->ast->program.recent;
    static const char* format = "(\033[1;33m%s\033[0m) %-*s%s\n\n";

    switch (node->type) {
        case AST_PROGRAM:
            break;
        case AST_FUNCTION:
            fprintf(stderr, format, "F+", ALIGNMENT, "Function", node->function.identifier);
            break;
        case AST_VAR_DEF:
            fprintf(stderr, format, "V+", ALIGNMENT, "VarDef", node->vardef.identifier);
            break;
        case AST_VAR:
            break;
        case AST_ASSIGNMENT:
            break;
        default:
            break;
    }
}

// =======================================================

const char* str_token(const Token* token, const TokenStringType mode, const SharedData* data)
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
        [IDENTIFIER] = get_identifier(data, token->attribute_str_idx),
        [MAIN_IDENTIFIER] = get_identifier(data, token->attribute_str_idx),
        [STRING] = get_string(data, token->attribute_str_idx),
        [STRING_INTERPOLATION] = get_string(data, token->attribute_str_idx),
        [STRING_INTERPOLATION_END] = get_string(data, token->attribute_str_idx),
        [MULTILINE_STRING] = get_string(data, token->attribute_str_idx),
        [MULTILINE_STRING_INTERPOLATION] = get_string(data, token->attribute_str_idx),
        [MULTILINE_STRING_INTERPOLATION_END] = get_string(data, token->attribute_str_idx),
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

void print_token(const Token* token, const SharedData* data)
{
    const char* name = str_token(token, GET_NAME, data);
    const char* attribute = str_token(token, GET_ATTRIBUTE, data);

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
