/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    debug.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "debug.h"
#include "scanner.h"
#include "shared_data.h"

#define ALIGNMENT 34

static void escaped_print(const char* string)
{
    if (string == NULL) {
        return;
    }

    int idx = 0;
    while (string[idx] != '\0') {
        if (string[idx] == '\n')
            fprintf(stderr, "\\n");
        else
            fprintf(stderr, "%c", string[idx]);
        idx++;
    }
}

void print_token(const Token* token, const SharedData* data)
{
    switch (token->type) {
        case NONE:
            fprintf(stderr, "%-*s\n", ALIGNMENT, "NONE");
            break;
        case ERROR:
            fprintf(stderr, "%-*s%i\n", ALIGNMENT, "ERROR", token->attribute_error);
            break;
        case INTEGER:
            fprintf(stderr, "%-*s%li\n", ALIGNMENT, "INTEGER", token->attribute_int);
            break;
        case DOUBLE:
            fprintf(stderr, "%-*s%lf\n", ALIGNMENT, "DOUBLE", token->attribute_double);
            break;
        case OPERATOR:
            switch (token->attribute_operation_type) {
                case PLUS:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "OPERATOR", "+");
                    break;
                case MINUS:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "OPERATOR", "-");
                    break;
                case MULTIPLY:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "OPERATOR", "*");
                    break;
                case AND:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "OPERATOR", "&&");
                    break;
                case OR:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "OPERATOR", "||");
                    break;
                case NOT:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "OPERATOR", "!");
                    break;
                case DIVIDE:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "OPERATOR", "/");
                    break;
                default:
                    fprintf(stderr, "%-*s%i\n", ALIGNMENT, "OPERATOR",
                        token->attribute_operation_type);
                    break;
            }
            break;
        case IDENTIFIER:
            fprintf(stderr, "%-*s%s\n", ALIGNMENT, "IDENTIFIER",
                get_identifier(data, token->attribute_str_idx));
            break;
        case KEYWORD:
            switch (token->attribute_keyword) {
                case DEF_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "def");
                    break;
                case DO_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "do");
                    break;
                case DOUBLE_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "Double");
                    break;
                case ELSE_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "else");
                    break;
                case IF_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "if");
                    break;
                case IMPORT_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "import");
                    break;
                case INT_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "Int");
                    break;
                case RETURN_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "return");
                    break;
                case STRING_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "String");
                    break;
                case THEN_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "then");
                    break;
                case UNIT_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "Unit");
                    break;
                case VAL_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "val");
                    break;
                case VAR_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "var");
                    break;
                case WHILE_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "while");
                    break;
                case BOOLEAN_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "Boolean");
                    break;
                case CASE_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "case");
                    break;
                case CLASS_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "class");
                    break;
                case EXTENSION_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "extension");
                    break;
                case FOR_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "for");
                    break;
                case LIST_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "List");
                    break;
                case MATCH_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "match");
                    break;
                case TYPE_KW:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "KEYWORD", "type");
                    break;
                default:
                    fprintf(stderr, "%-*s%i\n", ALIGNMENT, "KEYWORD",
                        token->attribute_keyword);
                    break;
            }
            break;
        case BOOLEAN:
            fprintf(stderr, "%-*s%s\n", ALIGNMENT, "BOOLEAN",
                token->attribute_bool ? "true" : "false");
            break;

        // STRINGS
        case STRING_INTERPOLATION:
            fprintf(stderr, "%-*s", ALIGNMENT, "STRING_INTERPOLATION");
            escaped_print(get_string(data, token->attribute_str_idx));
            fprintf(stderr, "\n");
            break;
        case STRING_INTERPOLATION_END:
            fprintf(stderr, "%-*s", ALIGNMENT, "STRING_INTERPOLATION_END");
            escaped_print(get_string(data, token->attribute_str_idx));
            fprintf(stderr, "\n");
            break;
        case MULTILINE_STRING_INTERPOLATION:
            fprintf(stderr, "%-*s", ALIGNMENT, "MULTILINE_STRING_INTERPOLATION");
            escaped_print(get_string(data, token->attribute_str_idx));
            fprintf(stderr, "\n");
            break;
        case MULTILINE_STRING_INTERPOLATION_END:
            fprintf(stderr, "%-*s", ALIGNMENT, "MULTILINE_STRING_INTERPOLATION_END");
            escaped_print(get_string(data, token->attribute_str_idx));
            fprintf(stderr, "\n");
            break;
        case STRING:
            fprintf(stderr, "%-*s", ALIGNMENT, "STRING");
            escaped_print(get_string(data, token->attribute_str_idx));
            fprintf(stderr, "\n");
            break;
        case MULTILINE_STRING:
            fprintf(stderr, "%-*s", ALIGNMENT, "MULTILINE_STRING");
            fprintf(stderr, "\n");
            break;

        case DOT:
            fprintf(stderr, "%-*s.\n", ALIGNMENT, "DOT");
            break;
        case DECLARATION_SIGN:
            fprintf(stderr, "%-*s:\n", ALIGNMENT, "DECLARATION_SIGN");
            break;
        case COMMA:
            fprintf(stderr, "%-*s,\n", ALIGNMENT, "COMMA");
            break;
        case INDENT:
            fprintf(stderr, "%-*s\n", ALIGNMENT, "INDENT");
            break;
        case LINE_FEED:
            fprintf(stderr, "%-*s\n", ALIGNMENT, "LINE_FEED");
            break;
        case WHITESPACE:
            fprintf(stderr, "%-*s\n", ALIGNMENT, "WHITESPACE");
            break;
        case ASSIGNMENT:
            fprintf(stderr, "%-*s=\n", ALIGNMENT, "ASSIGNMENT");
            break;
        case COMPARE:
            switch (token->attribute_compare_type) {
                case EQUAL:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "COMPARE", "==");
                    break;
                case NOT_EQUAL:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "COMPARE", "!=");
                    break;
                case LESS:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "COMPARE", "<");
                    break;
                case GREATER:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "COMPARE", ">");
                    break;
                case LESS_EQUAL:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "COMPARE", "<=");
                    break;
                case GREATER_EQUAL:
                    fprintf(stderr, "%-*s%s\n", ALIGNMENT, "COMPARE", ">=");
                    break;
                default:
                    fprintf(stderr, "%-*s%i\n", ALIGNMENT, "COMPARE",
                        token->attribute_compare_type);
                    break;
            }
            break;
        case LEFT_BRACKET:
            fprintf(stderr, "%-*s(\n", ALIGNMENT, "LEFT_BRACKET");
            break;
        case RIGHT_BRACKET:
            fprintf(stderr, "%-*s)\n", ALIGNMENT, "RIGHT_BRACKET");
            break;
        case END_OF_FILE:
            fprintf(stderr, "%-*s\n", ALIGNMENT, "END_OF_FILE");
            break;
        default:
            fprintf(stderr, "%-*s%i\n", ALIGNMENT, "UNKNOWN_TOKEN", token->type);
            break;
    }
}
