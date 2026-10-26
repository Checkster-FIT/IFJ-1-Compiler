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
        printf("");
        return;
    }

    int idx = 0;
    while (string[idx] != '\0') {
        if (string[idx] == '\n')
            printf("\\n");
        else
            printf("%c", string[idx]);
        idx++;
    }
}

void print_token(const Token* token, const SharedData* data)
{
    switch (token->type) {
        case NONE:
            printf("%-*s\n", ALIGNMENT, "NONE");
            break;
        case ERROR:
            printf("%-*s%i\n", ALIGNMENT, "ERROR", token->attribute_error);
            break;
        case INTEGER:
            printf("%-*s%li\n", ALIGNMENT, "INTEGER", token->attribute_int);
            break;
        case DOUBLE:
            printf("%-*s%lf\n", ALIGNMENT, "DOUBLE", token->attribute_double);
            break;
        case OPERATOR:
            switch (token->attribute_operation_type) {
                case PLUS:
                    printf("%-*s%s\n", ALIGNMENT, "OPERATOR", "+");
                    break;
                case MINUS:
                    printf("%-*s%s\n", ALIGNMENT, "OPERATOR", "-");
                    break;
                case MULTIPLY:
                    printf("%-*s%s\n", ALIGNMENT, "OPERATOR", "*");
                    break;
                case AND:
                    printf("%-*s%s\n", ALIGNMENT, "OPERATOR", "&&");
                    break;
                case OR:
                    printf("%-*s%s\n", ALIGNMENT, "OPERATOR", "||");
                    break;
                case NOT:
                    printf("%-*s%s\n", ALIGNMENT, "OPERATOR", "!");
                    break;
                case DIVIDE:
                    printf("%-*s%s\n", ALIGNMENT, "OPERATOR", "/");
                    break;
                default:
                    printf("%-*s%i\n", ALIGNMENT, "OPERATOR",
                        token->attribute_operation_type);
                    break;
            }
            break;
        case IDENTIFIER:
            printf("%-*s%s\n", ALIGNMENT, "IDENTIFIER",
                get_identifier(data, token->attribute_str_idx));
            break;
        case KEYWORD:
            switch (token->attribute_keyword) {
                case DEF_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "def");
                    break;
                case DO_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "do");
                    break;
                case DOUBLE_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "Double");
                    break;
                case ELSE_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "else");
                    break;
                case IF_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "if");
                    break;
                case IMPORT_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "import");
                    break;
                case INT_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "Int");
                    break;
                case RETURN_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "return");
                    break;
                case STRING_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "String");
                    break;
                case THEN_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "then");
                    break;
                case UNIT_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "Unit");
                    break;
                case VAL_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "val");
                    break;
                case VAR_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "var");
                    break;
                case WHILE_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "while");
                    break;
                case BOOLEAN_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "Boolean");
                    break;
                case CASE_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "case");
                    break;
                case CLASS_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "class");
                    break;
                case EXTENSION_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "extension");
                    break;
                case FOR_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "for");
                    break;
                case LIST_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "List");
                    break;
                case MATCH_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "match");
                    break;
                case TYPE_KW:
                    printf("%-*s%s\n", ALIGNMENT, "KEYWORD", "type");
                    break;
                default:
                    printf("%-*s%i\n", ALIGNMENT, "KEYWORD",
                        token->attribute_keyword);
                    break;
            }
            break;
        case BOOLEAN:
            printf("%-*s%s\n", ALIGNMENT, "BOOLEAN",
                token->attribute_bool ? "true" : "false");
            break;

        // STRINGS
        case STRING_INTERPOLATION:
            printf("%-*s", ALIGNMENT, "STRING_INTERPOLATION");
            escaped_print(get_string(data, token->attribute_str_idx));
            printf("\n");
            break;
        case STRING_INTERPOLATION_END:
            printf("%-*s", ALIGNMENT, "STRING_INTERPOLATION_END");
            escaped_print(get_string(data, token->attribute_str_idx));
            printf("\n");
            break;
        case MULTILINE_STRING_INTERPOLATION:
            printf("%-*s", ALIGNMENT, "MULTILINE_STRING_INTERPOLATION");
            escaped_print(get_string(data, token->attribute_str_idx));
            printf("\n");
            break;
        case MULTILINE_STRING_INTERPOLATION_END:
            printf("%-*s", ALIGNMENT, "MULTILINE_STRING_INTERPOLATION_END");
            escaped_print(get_string(data, token->attribute_str_idx));
            printf("\n");
            break;
        case STRING:
            printf("%-*s", ALIGNMENT, "STRING");
            escaped_print(get_string(data, token->attribute_str_idx));
            printf("\n");
            break;
        case MULTILINE_STRING:
            printf("%-*s", ALIGNMENT, "MULTILINE_STRING");
            printf("\n");
            break;

        case DOT:
            printf("%-*s.\n", ALIGNMENT, "DOT");
            break;
        case DECLARATION_SIGN:
            printf("%-*s:\n", ALIGNMENT, "DECLARATION_SIGN");
            break;
        case COMMA:
            printf("%-*s,\n", ALIGNMENT, "COMMA");
            break;
        case INDENT:
            printf("%-*s\n", ALIGNMENT, "INDENT");
            break;
        case LINE_FEED:
            printf("%-*s\n", ALIGNMENT, "LINE_FEED");
            break;
        case WHITESPACE:
            printf("%-*s\n", ALIGNMENT, "WHITESPACE");
            break;
        case ASSIGNMENT:
            printf("%-*s=\n", ALIGNMENT, "ASSIGNMENT");
            break;
        case COMPARE:
            switch (token->attribute_compare_type) {
                case EQUAL:
                    printf("%-*s%s\n", ALIGNMENT, "COMPARE", "==");
                    break;
                case NOT_EQUAL:
                    printf("%-*s%s\n", ALIGNMENT, "COMPARE", "!=");
                    break;
                case LESS:
                    printf("%-*s%s\n", ALIGNMENT, "COMPARE", "<");
                    break;
                case GREATER:
                    printf("%-*s%s\n", ALIGNMENT, "COMPARE", ">");
                    break;
                case LESS_EQUAL:
                    printf("%-*s%s\n", ALIGNMENT, "COMPARE", "<=");
                    break;
                case GREATER_EQUAL:
                    printf("%-*s%s\n", ALIGNMENT, "COMPARE", ">=");
                    break;
                default:
                    printf("%-*s%i\n", ALIGNMENT, "COMPARE",
                        token->attribute_compare_type);
                    break;
            }
            break;
        case LEFT_BRACKET:
            printf("%-*s(\n", ALIGNMENT, "LEFT_BRACKET");
            break;
        case RIGHT_BRACKET:
            printf("%-*s)\n", ALIGNMENT, "RIGHT_BRACKET");
            break;
        case END_OF_FILE:
            printf("%-*s\n", ALIGNMENT, "END_OF_FILE");
            break;
        default:
            printf("%-*s%i\n", ALIGNMENT, "UNKNOWN_TOKEN", token->type);
            break;
    }
}
