/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    scanner.c
 * @brief   Implementation of get_token().
 * @author  Drahovska Andrea (xdrahoa00), Wyslych Vojtech (xwyslyv00)
 */

#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>

int init_scanner(ScannerContext* scanner_context)
{
    ScannedString* scanned_string = init_scanned_string();
    if (!scanned_string)
        return 1;

    scanner_context->beginning_of_file = true;
    scanner_context->inside_interpolated_string = false;
    scanner_context->scanned_string = scanned_string;
    scanner_context->scanned_char_buffer = '\0';
    scanner_context->read_buffer = false;

    return 0;
}

ScannedString* init_scanned_string()
{
    const size_t scanned_string_len = 64;

    ScannedString* string_struct = malloc(sizeof(ScannedString));
    char* string_chars = malloc(sizeof(char) * scanned_string_len);

    if (!string_struct || !string_chars) {
        free(string_struct);
        free(string_chars);
        return NULL;
    }

    string_struct->chars = string_chars;
    string_struct->chars[0] = '\0';
    string_struct->len = 0;
    string_struct->capacity = scanned_string_len;

    return string_struct;
}

int resize_scanned_string(ScannedString* string)
{
    const size_t new_capacity = string->capacity * 2;
    char* new_string = realloc(string->chars, sizeof(char) * new_capacity);
    if (!new_string) {
        // realloc error
        return 1;
    }

    string->capacity = new_capacity;
    string->chars = new_string;

    return 0;
}

void clear_scanned_string(ScannedString* string)
{
    string->len = 0;
    string->chars[0] = '\0';
}

int scanned_string_append(ScannedString* string, char scanned_char)
{
    if (string->len + 1 >= string->capacity) {
        int result = resize_scanned_string(string);
        if (result == 1)
            return 1;
    }

    string->chars[string->len] = scanned_char;
    string->len += 1;
    string->chars[string->len] = '\0';

    return 0;
}

void scanned_string_remove_last(ScannedString* string)
{
    if (string->len == 0)
        return; // nothing to be removed

    string->len -= 1;
    string->chars[string->len] = '\0';
}

char scanned_string_top(const ScannedString* string)
{
    // empty string
    if (string->len == 0)
        return '\0';

    return string->chars[(string->len) - 1];
}

void dispose_scanned_string(ScannedString* string)
{
    if (!string)
        return;
    free(string->chars);
    free(string);
}

Token get_token(ScannerContext* context, FILE* stream)
{
    State state = STATE_START;
    Token token;

    token.type = ERROR; // default setting to ERROR
    token.attribute_error_type = LEXICAL_ERROR;

    clear_scanned_string(context->scanned_string);

    while (1) {

        char c;

        if (context->read_buffer) {
            c = context->scanned_char_buffer;
        } else {
            c = (char)fgetc(stream);

            if (ferror(stream)) {
                token.type = ERROR;
                token.attribute_error_type = INTERNAL_ERROR;
                return token;
            }

            context->scanned_char_buffer = c;
        }

        context->read_buffer = false; // default setting

        // FSM transitions
        switch (state) {

            /*TODO:
             * r_curly_bracket:interpolo=true
             * STATE_AT_SYMBOL - STATE_END_OF_FILE
             */
            case STATE_START:

                if (context->inside_interpolated_string == true && c != EOF && c != '\n' && c != '"')
                    state = STATE_STRING_INTERPOLATION;
                else if (context->inside_interpolated_string == true && c == '"')
                    state = STATE_STRING_INTERPOLATION_END;
                else if (c == '&')
                    state = STATE_AMPERSAND_1;
                else if (c == '|')
                    state = STATE_OR_1;
                else if (c == '+' || c == '-' || c == '*' || c == '!')
                    state = STATE_OPERATOR;
                else if (c == 's')
                    state = STATE_CHARACTER_S;
                else if ((c >= 'a' && c <= 'r') || (c >= 't' && c <= 'z') || (c >= 'A' && c <= 'Z'))
                    state = STATE_IDENTIFIER;
                else if (c == '_')
                    state = STATE_UNDERSCORE_IDENTIFIER;
                else if (c == '"')
                    state = STATE_STRING_WHICH;
                else if (c >= '1' && c <= '9')
                    state = STATE_INTEGER;
                else if (c == '.')
                    state = STATE_DOT;
                else if (c == '0')
                    state = STATE_ZERO;
                else if (c == ':')
                    state = STATE_COLON;
                else if (c == '@')
                    state = STATE_AT_SYMBOL;
                else if (c == ',')
                    state = STATE_COMMA;
                else if (c == '\n')
                    state = STATE_LINE_FEED;
                else if (c == '/')
                    state = STATE_COMMENT_START;
                else if (c == '<' || c == '>')
                    state = STATE_LESSER_GREATER;
                else if (c == '=')
                    state = STATE_ASSIGNMENT;
                else if (c == '(')
                    state = STATE_LEFT_BRACKET;
                else if (c == ')')
                    state = STATE_RIGHT_BRACKET;
                else if (c == '}')
                    state = STATE_RIGHT_CURLY_BRACKET;
                else if (c == ' ' && context->beginning_of_file == true)
                    state = STATE_INDENT;
                else if (c == EOF)
                    state = STATE_END_OF_FILE;

                else {
                    return token; // invalid character
                }

                break;

            case STATE_AMPERSAND_1:

                if (c == '&')
                    state = STATE_OPERATOR;
                else {
                    return token;
                }

                break;

            case STATE_OR_1:

                if (c == '|')
                    state = STATE_OPERATOR;
                else {
                    return token;
                }

                break;

            case STATE_OPERATOR:

                token.type = OPERATOR;

                char a = scanned_string_top(context->scanned_string);
                if (a == '+')
                    token.attribute_operation_type = PLUS;
                else if (a == '-')
                    token.attribute_operation_type = MINUS;
                else if (a == '*')
                    token.attribute_operation_type = MULTIPLY;
                else if (a == '/')
                    token.attribute_operation_type = DIVIDE;
                else if (a == '&')
                    token.attribute_operation_type = AND;
                else if (a == '|')
                    token.attribute_operation_type = OR;
                else if (a == '!')
                    token.attribute_operation_type = NOT;

                else {
                    token.type = ERROR;
                    token.attribute_error_type = INTERNAL_ERROR;
                }

                RETURN_TOKEN_AND_SET_READ_BUFFER

            case STATE_STRING_WHICH:

                // remove `"` from beginning of the scanned string since parser only needs the inside of the string
                scanned_string_remove_last(context->scanned_string);

                if (c == '"')
                    state = STATE_EMPTY_STRING;
                else if (c == '\\')
                    state = STATE_STRING_ESCAPE;
                else if (c != '\n' && c != EOF && c != '\\')
                    state = STATE_STRING_BEGIN;
                else {
                    // TODO: error - invalid string; a simple string cannot contain multiple lines OR EOF
                    return token;
                }

                break;

            case STATE_STRING_BEGIN:

                if (c == '"')
                    state = STATE_STRING_END;
                else if (c == '\\')
                    state = STATE_STRING_ESCAPE;
                else if (c != '\n' && c != EOF && c != '\\')
                    state = STATE_STRING_BEGIN;
                else {
                    // TODO: error - invalid string; a simple string cannot contain multiple lines OR EOF
                    return token;
                }

                break;

            case STATE_STRING_ESCAPE:

                if (c != '\n' && c != EOF)
                    state = STATE_STRING_BEGIN;
                else {
                    // TODO: error - invalid string; a simple string cannot contain multiple lines OR EOF
                    return token;
                }

                break;

            case STATE_STRING_END:

                scanned_string_remove_last(context->scanned_string);
                token.type = STRING;
                // TODO: how to pass s string to parser?
                token.attribute_str_ptr = NULL;

                RETURN_TOKEN_AND_SET_READ_BUFFER

            case STATE_EMPTY_STRING:

                if (c == '"') {
                    clear_scanned_string(context->scanned_string);
                    state = STATE_STRING_MULTILINE_START;
                    // we dont want `"` to be saved into scanned string to we continue to the next char from the stream
                    continue;
                }

                token.type = EMPTY_STRING;
                token.attribute_none = NULL;

                RETURN_TOKEN_AND_SET_READ_BUFFER

            case STATE_STRING_MULTILINE_START:

                if (c == '"')
                    state = STATE_STRING_MULTILINE_END_1;
                else if (c != EOF && c != '"')
                    state = STATE_STRING_MULTILINE_START;
                else {
                    return token;
                }

                break;

            case STATE_STRING_MULTILINE_END_1:

                if (c == '"')
                    state = STATE_STRING_MULTILINE_END_2;
                else if (c != EOF && c != '"')
                    state = STATE_STRING_MULTILINE_START;
                else {
                    return token;
                }

                break;

            case STATE_STRING_MULTILINE_END_2:

                if (c == '"')
                    state = STATE_STRING_MULTILINE_END_3;
                else if (c != EOF && c != '"')
                    state = STATE_STRING_MULTILINE_START;
                else {
                    return token;
                }

                break;

            case STATE_STRING_MULTILINE_END_3:

                // remove the ending `"""` from the string
                scanned_string_remove_last(context->scanned_string);
                scanned_string_remove_last(context->scanned_string);
                scanned_string_remove_last(context->scanned_string);

                token.type = MULTILINE_STRING;
                // TODO: how to pass s string to parser?
                token.attribute_str_ptr = NULL;

                RETURN_TOKEN_AND_SET_READ_BUFFER

            default:
                return token;
        }

        // put character into the buffer
        int result = scanned_string_append(context->scanned_string, c);

        if (result == 1) {
            // memory allocation fail
            token.type = ERROR;
            token.attribute_error_type = INTERNAL_ERROR;
            return token;
        }

        context->beginning_of_file = false;
    }
}