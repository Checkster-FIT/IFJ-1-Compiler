/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    debug.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "debug.h"
#include "scanner.h"
#include "shared_data.h"

void print_token(const Token* token, const SharedData* data)
{
    switch (token->type) {
        case NONE:
            printf("NONE\n");
            break;
        case ERROR:
            printf("ERROR [%i]\n", token->attribute_error);
            break;
        case INTEGER:
            printf("INTEGER [%li]\n", token->attribute_int);
            break;
        case DOUBLE:
            printf("DOUBLE [%lf]\n", token->attribute_double);
            break;
        case OPERATOR:
            switch (token->attribute_operation_type) {
                case PLUS:
                    printf("OPERATOR [+]\n");
                    break;
                case MINUS:
                    printf("OPERATOR [-]\n");
                    break;
                case MULTIPLY:
                    printf("OPERATOR [*]\n");
                    break;
                case AND:
                    printf("OPERATOR [&&]\n");
                    break;
                case OR:
                    printf("OPERATOR [||]\n");
                    break;
                case NOT:
                    printf("OPERATOR [!]\n");
                    break;
                case DIVIDE:
                    printf("OPERATOR [/]\n");
                    break;
                default:
                    printf("OPERATOR [%i]\n", token->attribute_operation_type);
                    break;
            }
            break;
        case IDENTIFIER:
            printf("IDENTIFIER [%s]\n",
                get_identifier(data, token->attribute_str_idx));
            break;
        case KEYWORD:
            switch (token->attribute_keyword) {
                case DEF_KW:
                    printf("KEYWORD [def]\n");
                    break;
                case DO_KW:
                    printf("KEYWORD [do]\n");
                    break;
                case DOUBLE_KW:
                    printf("KEYWORD [Double]\n");
                    break;
                case ELSE_KW:
                    printf("KEYWORD [else]\n");
                    break;
                case IF_KW:
                    printf("KEYWORD [if]\n");
                    break;
                case IMPORT_KW:
                    printf("KEYWORD [import]\n");
                    break;
                case INT_KW:
                    printf("KEYWORD [Int]\n");
                    break;
                case RETURN_KW:
                    printf("KEYWORD [return]\n");
                    break;
                case STRING_KW:
                    printf("KEYWORD [String]\n");
                    break;
                case THEN_KW:
                    printf("KEYWORD [then]\n");
                    break;
                case UNIT_KW:
                    printf("KEYWORD [Unit]\n");
                    break;
                case VAL_KW:
                    printf("KEYWORD [val]\n");
                    break;
                case VAR_KW:
                    printf("KEYWORD [var]\n");
                    break;
                case WHILE_KW:
                    printf("KEYWORD [while]\n");
                    break;
                case BOOLEAN_KW:
                    printf("KEYWORD [Boolean]\n");
                    break;
                case CASE_KW:
                    printf("KEYWORD [case]\n");
                    break;
                case CLASS_KW:
                    printf("KEYWORD [class]\n");
                    break;
                case EXTENSION_KW:
                    printf("KEYWORD [extension]\n");
                    break;
                case FOR_KW:
                    printf("KEYWORD [for]\n");
                    break;
                case LIST_KW:
                    printf("KEYWORD [List]\n");
                    break;
                case MATCH_KW:
                    printf("KEYWORD [match]\n");
                    break;
                case TYPE_KW:
                    printf("KEYWORD [type]\n");
                    break;
                default:
                    printf("KEYWORD [%i]\n", token->attribute_keyword);
                    break;
            }
            break;
        case BOOLEAN:
            if (token->attribute_bool)
                printf("BOOLEAN [true]\n");
            else
                printf("BOOLEAN [false]\n");
            break;
        case STRING_INTERPOLATION:
            printf("STRING_INTERPOLATION [%s]\n", get_string(data, token->attribute_str_idx));
            break;
        case STRING_INTERPOLATION_END:
            printf("STRING_INTERPOLATION_END [%s]\n", get_string(data, token->attribute_str_idx));
            break;
        case MULTILINE_STRING_INTERPOLATION:
            printf("MULTILINE_STRING_INTERPOLATION [%s]\n", get_string(data, token->attribute_str_idx));
            break;
        case MULTILINE_STRING_INTERPOLATION_END:
            printf("MULTILINE_STRING_INTERPOLATION_END [%s]\n", get_string(data, token->attribute_str_idx));
            break;
        case STRING:
            printf("STRING [%s]\n", get_string(data, token->attribute_str_idx));
            break;
        case MULTILINE_STRING:
            printf("MULTILINE_STRING [%s]\n", get_string(data, token->attribute_str_idx));
            break;
        case DOT:
            printf("DOT [.]\n");
            break;
        case DECLARATION_SIGN:
            printf("DECLARATION_SIGN [:]\n");
            break;
        case COMMA:
            printf("COMMA [,]\n");
            break;
        case INDENT:
            printf("INDENT\n");
            break;
        case LINE_FEED:
            printf("LINE_FEED\n");
            break;
        case WHITESPACE:
            printf("WHITESPACE\n");
            break;
        case ASSIGNMENT:
            printf("ASSIGNMENT [=]\n");
            break;
        case COMPARE:
            switch (token->attribute_compare_type) {
                case EQUAL:
                    printf("COMPARE [==]\n");
                    break;
                case NOT_EQUAL:
                    printf("COMPARE [!=]\n");
                    break;
                case LESS:
                    printf("COMPARE [<]\n");
                    break;
                case GREATER:
                    printf("COMPARE [>]\n");
                    break;
                case LESS_EQUAL:
                    printf("COMPARE [<=]\n");
                    break;
                case GREATER_EQUAL:
                    printf("COMPARE [>=]\n");
                    break;
                default:
                    printf("COMPARE [%i]\n", token->attribute_compare_type);
                    break;
            }
            break;
        case LEFT_BRACKET:
            printf("LEFT_BRACKET [(]\n");
            break;
        case RIGHT_BRACKET:
            printf("RIGHT_BRACKET [)]\n");
            break;
        case END_OF_FILE:
            printf("END_OF_FILE\n");
            break;
        default:
            printf("Token [%i]\n", token->type);
            break;
    }
}
