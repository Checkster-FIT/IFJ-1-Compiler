/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    scanner.c
 * @brief   Implementation of get_token().
 * @author  Drahovska Andrea (xdrahoa00), Wyslych Vojtech (xwyslyv00)
 */

#include "scanner.h"
#include "error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* keywords[NUMBER_OF_KEYWORDS] = {
    "def",
    "do",
    "Double",
    "else",
    "if",
    "import",
    "Int",
    "return",
    "String",
    "then",
    "Unit",
    "val",
    "var",
    "while",
    "Boolean",
    "case",
    "class",
    "extension",
    "for",
    "List",
    "match",
    "type",
    "true",
    "false"
};

int init_scanner(ScannerContext* scanner_context)
{
    ScannedString* scanned_string = init_scanned_string();
    if (!scanned_string)
        return 1;

    scanner_context->beginning_of_file = true;
    scanner_context->interpolation = NO_INTERPOLATION;
    scanner_context->master_interpolated_string = NO_INTERPOLATION;
    scanner_context->scanned_string = scanned_string;
    scanner_context->scanned_char_buffer = '\0';
    scanner_context->read_buffer = false;
    scanner_context->scanning_variable_inside_string_interpolation = false;

    return 0;
}

void destroy_scanner(const ScannerContext* scanner_context)
{
    dispose_scanned_string(scanner_context->scanned_string);
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

int check_identifier_for_kw(ScannedString* string)
{
    if (!string)
        return -2;

    for (int i = 0; i < NUMBER_OF_KEYWORDS; i++) {
        if (strcmp(string->chars, keywords[i]) == 0)
            return i;
    }

    return IS_IDENTIFIER;
}

/** TODO:
 *
 * inline comments after new line send multiple new line tokens
 *
 * indent, dedent - for each block, use context to track number of dedents
 *
 */
Token get_token(ScannerContext* context, SharedData* data, FILE* stream)
{
    State state = STATE_START;
    Token token;

    token.type = ERROR; // default setting to ERROR
    token.attribute_error = 1; // lexical error exit code

    clear_scanned_string(context->scanned_string);

    while (1) {

        char c;
        int result;
        char* end_ptr;

        if (context->read_buffer) {
            c = context->scanned_char_buffer;
        } else {
            c = (char)fgetc(stream);

            if (ferror(stream)) {
                token.type = ERROR;
                token.attribute_error = print_error(STREAM_READ_ERROR);
                return token;
            }

            context->scanned_char_buffer = c;
        }

        context->read_buffer = false; // default setting

        // FSM transitions
        switch (state) {
            case STATE_START:

                if (context->scanning_variable_inside_string_interpolation == true && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
                    // handle the variable inside interpolated string as normal variable, then return back into interpolated string
                    context->scanning_variable_inside_string_interpolation = false;
                    state = STATE_IDENTIFIER;
                } else if (context->scanning_variable_inside_string_interpolation == true && c == '_') {
                    // handle the variable inside interpolated string as normal variable, then return back into interpolated string
                    context->scanning_variable_inside_string_interpolation = false;
                    state = STATE_UNDERSCORE_IDENTIFIER;
                } else if (context->interpolation == SIMPLE_INTERPOLATED_STRING && c == '"')
                    state = STATE_STRING_INTERPOLATION_END;
                else if (context->interpolation == SIMPLE_INTERPOLATED_STRING && c == '$')
                    state = STATE_STRING_INTERPOLATION_DOLLAR;
                else if (context->interpolation == SIMPLE_INTERPOLATED_STRING && c != EOF && c != '\n' && c != '"')
                    state = STATE_STRING_INTERPOLATION;
                else if (context->interpolation == MULTILINE_INTERPOLATED_STRING && c == '"')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_END_1;
                else if (context->interpolation == MULTILINE_INTERPOLATED_STRING && c == '$')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_DOLLAR;
                else if (context->interpolation == MULTILINE_INTERPOLATED_STRING && c != EOF && c != '\n' && c != '"')
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                else if (c == '&')
                    state = STATE_AMPERSAND_1;
                else if (c == '|')
                    state = STATE_OR_1;
                else if (c == '+' || c == '-' || c == '*')
                    state = STATE_OPERATOR;
                else if (c == '!')
                    state = STATE_EXCLAMATION_SYMBOL;
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
                    state = STATE_SLASH;
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
                else if (c == ' ' && context->beginning_of_file == false)
                    continue;
                else if (c == EOF) {
                    token.type = END_OF_FILE;
                    token.attribute_none = NULL;
                    return token;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_NO_TRANSITION, state, c);
                    return token;
                }

                break;

            // OPERATOR
            case STATE_AMPERSAND_1:

                if (c == '&')
                    state = STATE_OPERATOR;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_UNKNOWN_OPERATOR, context->scanned_string->chars, "&&");
                    return token;
                }

                break;

            case STATE_OR_1:

                if (c == '|')
                    state = STATE_OPERATOR;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_UNKNOWN_OPERATOR, context->scanned_string->chars, "||");
                    return token;
                }

                break;

            case STATE_SLASH:
                if (c == '/')
                    state = STATE_COMMENT_START;
                else if (c == '*')
                    state = STATE_BLOCK_COMMENT_START;
                else {
                    token.type = OPERATOR;
                    token.attribute_operation_type = DIVIDE;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_EXCLAMATION_SYMBOL:
                if (c == '=')
                    state = STATE_COMPARE;
                else {
                    token.type = OPERATOR;
                    token.attribute_operation_type = NOT;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
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
                else if (a == '&')
                    token.attribute_operation_type = AND;
                else if (a == '|')
                    token.attribute_operation_type = OR;

                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(CORRUPTED_MEMORY, state);
                    return token;
                }

                RETURN_TOKEN_AND_SET_READ_BUFFER

            // STRING
            case STATE_STRING_WHICH:

                // remove `"` from beginning of the scanned string since parser only needs the inside of the string
                scanned_string_remove_last(context->scanned_string);

                if (c == '"')
                    state = STATE_EMPTY_STRING;
                else if (c == '\\')
                    state = STATE_STRING_ESCAPE;
                else if (c != '\n' && c != EOF && c != '\\' && c != '"')
                    state = STATE_STRING_BEGIN;
                else if (c == '\n') {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "a new line character", context->scanned_string->chars);
                    return token;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_STRING_BEGIN:

                if (c == '"')
                    state = STATE_STRING_END;
                else if (c == '\\')
                    state = STATE_STRING_ESCAPE;
                else if (c != '\n' && c != EOF && c != '\\' && c != '"')
                    state = STATE_STRING_BEGIN;
                else if (c == '\n') {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "a new line character", context->scanned_string->chars);
                    return token;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_STRING_ESCAPE:

                scanned_string_remove_last(context->scanned_string); // remove '\'

                if (c == 'n') {
                    state = STATE_STRING_BEGIN;
                    result = scanned_string_append(context->scanned_string, '\n');
                    if (result == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "scanned_string");
                        return token;
                    }
                    continue; // ignore 'n'

                } else if (c != '\n' && c != EOF)
                    state = STATE_STRING_BEGIN;
                else if (c == '\n') {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "a new line character", context->scanned_string->chars);
                    return token;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_STRING_END:

                scanned_string_remove_last(context->scanned_string);
                token.type = STRING;

                result = add_string(data, context->scanned_string->chars);
                if (result == -1) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "scanned string");

                    return token;
                }

                token.attribute_str_idx = result;

                RETURN_TOKEN_AND_SET_READ_BUFFER

            case STATE_EMPTY_STRING:

                if (c == '"') {
                    clear_scanned_string(context->scanned_string);
                    state = STATE_STRING_MULTILINE_START;
                    // we dont want `"` to be saved into scanned string to we continue to the next char from the stream
                    // (another possible solution would be adding another state something like STATE_STRING_MULTILINE_VERY_START which would remove the '"')
                    continue;
                }

                result = add_string(data, "");
                if (result == -1) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "scanned string");

                    return token;
                }

                token.type = STRING;
                token.attribute_str_idx = result;

                RETURN_TOKEN_AND_SET_READ_BUFFER

            case STATE_STRING_MULTILINE_START:

                if (c == '"')
                    state = STATE_STRING_MULTILINE_END_1;
                else if (c != EOF && c != '"')
                    state = STATE_STRING_MULTILINE_START;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_STRING_MULTILINE_END_1:

                if (c == '"')
                    state = STATE_STRING_MULTILINE_END_2;
                else if (c != EOF && c != '"')
                    state = STATE_STRING_MULTILINE_START;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_STRING_MULTILINE_END_2:

                if (c == '"')
                    state = STATE_STRING_MULTILINE_END_3;
                else if (c != EOF && c != '"')
                    state = STATE_STRING_MULTILINE_START;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_STRING_MULTILINE_END_3:

                // remove the ending `"""` from the string
                scanned_string_remove_last(context->scanned_string);
                scanned_string_remove_last(context->scanned_string);
                scanned_string_remove_last(context->scanned_string);

                token.type = MULTILINE_STRING;

                result = add_string(data, context->scanned_string->chars);
                if (result == -1) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "scanned multiline string");

                    return token;
                }

                token.attribute_str_idx = result;

                RETURN_TOKEN_AND_SET_READ_BUFFER

            // INTERPOLATED STRING
            case STATE_CHARACTER_S:

                if (c == '"')
                    state = STATE_STRING_INTERPOLATION_WHICH_1;
                else if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
                    state = STATE_IDENTIFIER;
                else {
                    token.type = IDENTIFIER;

                    result = add_identifier(data, context->scanned_string->chars);
                    if (result == -1) {
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "identifier");

                        return token;
                    }

                    token.attribute_str_idx = result;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_STRING_INTERPOLATION_WHICH_1:
                clear_scanned_string(context->scanned_string); // remove s
                clear_scanned_string(context->scanned_string); // remove "

                // can be overwritten by multiline interpolation
                // it is here to prevent code duplication as it would be needed to set in STATE_STRING_INTERPOLATION as well as in STATE_STRING_INTERPOLATION_DOLLAR
                // (interpolated string can start with $: s"$var")
                context->interpolation = SIMPLE_INTERPOLATED_STRING;
                context->master_interpolated_string = SIMPLE_INTERPOLATED_STRING;

                if (c == '"')
                    state = STATE_STRING_INTERPOLATION_WHICH_2;
                else if (c == '\n') {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "a new line character", context->scanned_string->chars);
                    return token;
                } else if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                } else if (c == '$')
                    state = STATE_STRING_INTERPOLATION_DOLLAR;
                else
                    state = STATE_STRING_INTERPOLATION;

                break;

            case STATE_STRING_INTERPOLATION_WHICH_2:
                scanned_string_remove_last(context->scanned_string); // removes "

                if (c == '"') {
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                    continue; // don't save the last "
                }

                context->interpolation = NO_INTERPOLATION;
                context->master_interpolated_string = NO_INTERPOLATION;
                token.type = STRING;
                result = add_string(data, "");
                if (result == -1) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "string interpolation");

                    return token;
                }

                token.attribute_str_idx = result;

                RETURN_TOKEN_AND_SET_READ_BUFFER;

            case STATE_MULTILINE_STRING_INTERPOLATION:
                context->interpolation = MULTILINE_INTERPOLATED_STRING;
                context->master_interpolated_string = MULTILINE_INTERPOLATED_STRING;
                if (c == '\\')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_ESCAPE;
                else if (c == '$')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_DOLLAR;
                else if (c == '"')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_END_1;
                else if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                } else
                    state = STATE_MULTILINE_STRING_INTERPOLATION;

                break;

            case STATE_MULTILINE_STRING_INTERPOLATION_ESCAPE:

                scanned_string_remove_last(context->scanned_string); // remove '\'

                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                }

                if (c == 'n') {
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                    continue; // ignore 'n'
                }

                state = STATE_MULTILINE_STRING_INTERPOLATION;

                break;

            // TODO:
            //   nested interpolation - simple string x multiline string

            case STATE_MULTILINE_STRING_INTERPOLATION_DOLLAR:

                scanned_string_remove_last(context->scanned_string); // removes $

                if (c == '$')
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                else if (c == '{')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_EXPRESSION;
                else {
                    // pointer to the whole string before $
                    token.type = MULTILINE_STRING_INTERPOLATION;

                    result = add_string(data, context->scanned_string->chars);
                    if (result == -1) {
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "multiline string interpolation");

                        return token;
                    }

                    token.attribute_str_idx = result;
                    context->scanning_variable_inside_string_interpolation = true;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_MULTILINE_STRING_INTERPOLATION_EXPRESSION:

                context->interpolation = NO_INTERPOLATION;

                scanned_string_remove_last(context->scanned_string); // removes { (dollar is already removed in the previous state)

                token.type = MULTILINE_STRING_INTERPOLATION;
                // pointer to the whole string before {
                result = add_string(data, context->scanned_string->chars);
                if (result == -1) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "multiline string interpolation");

                    return token;
                }

                token.attribute_str_idx = result;

                RETURN_TOKEN_AND_SET_READ_BUFFER;

            case STATE_MULTILINE_STRING_INTERPOLATION_END_1:
                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                } else if (c == '"') {
                    state = STATE_MULTILINE_STRING_INTERPOLATION_END_2;
                } else
                    state = STATE_MULTILINE_STRING_INTERPOLATION;

                break;

            case STATE_MULTILINE_STRING_INTERPOLATION_END_2:
                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                } else if (c == '"') {
                    state = STATE_MULTILINE_STRING_INTERPOLATION_END_3;
                } else
                    state = STATE_MULTILINE_STRING_INTERPOLATION;

                break;

            case STATE_MULTILINE_STRING_INTERPOLATION_END_3:
                context->interpolation = NO_INTERPOLATION;
                context->master_interpolated_string = NO_INTERPOLATION;

                // remove tailing """
                for (int i = 0; i < 3; i++) {
                    scanned_string_remove_last(context->scanned_string);
                }

                token.type = MULTILINE_STRING_INTERPOLATION_END;

                result = add_string(data, context->scanned_string->chars);
                if (result == -1) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "multiline string interpolation");

                    return token;
                }

                token.attribute_str_idx = result;

                RETURN_TOKEN_AND_SET_READ_BUFFER;

            case STATE_STRING_INTERPOLATION:
                if (c == '$')
                    state = STATE_STRING_INTERPOLATION_DOLLAR;
                else if (c == '"')
                    state = STATE_STRING_INTERPOLATION_END;
                else if (c == '\\')
                    state = STATE_STRING_INTERPOLATION_ESCAPE;
                else if (c != '\n' && c != EOF && c != '\\' && c != '"' && c != '$')
                    state = STATE_STRING_INTERPOLATION;
                else if (c == '\n') {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "a new line character", context->scanned_string->chars);
                    return token;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_STRING_INTERPOLATION_DOLLAR:

                scanned_string_remove_last(context->scanned_string); // removes $

                if (c == '$')
                    state = STATE_STRING_INTERPOLATION;
                else if (c == '{')
                    state = STATE_STRING_INTERPOLATION_EXPRESSION;
                else {
                    // pointer to the whole string before $
                    token.type = STRING_INTERPOLATION;

                    result = add_string(data, context->scanned_string->chars);
                    if (result == -1) {
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "string interpolation");

                        return token;
                    }

                    token.attribute_str_idx = result;
                    context->scanning_variable_inside_string_interpolation = true;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_STRING_INTERPOLATION_EXPRESSION:

                context->interpolation = NO_INTERPOLATION;

                scanned_string_remove_last(context->scanned_string); // removes {

                token.type = STRING_INTERPOLATION;
                // pointer to the whole string before {
                result = add_string(data, context->scanned_string->chars);
                if (result == -1) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "string interpolation");

                    return token;
                }

                token.attribute_str_idx = result;

                RETURN_TOKEN_AND_SET_READ_BUFFER;

            case STATE_STRING_INTERPOLATION_END:

                context->interpolation = NO_INTERPOLATION;
                context->master_interpolated_string = NO_INTERPOLATION;

                scanned_string_remove_last(context->scanned_string); // removes "

                token.type = STRING_INTERPOLATION_END;

                // pointer to the whole string before "
                result = add_string(data, context->scanned_string->chars);
                if (result == -1) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "string interpolation");

                    return token;
                }

                token.attribute_str_idx = result;

                RETURN_TOKEN_AND_SET_READ_BUFFER;

            case STATE_STRING_INTERPOLATION_ESCAPE:

                scanned_string_remove_last(context->scanned_string); // remove '\'

                if (c != '\n' && c != EOF)
                    state = STATE_STRING_INTERPOLATION;
                else if (c == 'n') {
                    state = STATE_STRING_INTERPOLATION;
                    continue; // ignore 'n'
                } else if (c == '\n') {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "a new line character", context->scanned_string->chars);
                    return token;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, "end of file", context->scanned_string->chars);
                    return token;
                }

                break;

            // IDENTIFIER / KEYWORD / BOOLEAN
            case STATE_UNDERSCORE_IDENTIFIER:

                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
                    state = STATE_IDENTIFIER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_IDENTIFIER, "_");
                    return token;
                }

                break;

            case STATE_IDENTIFIER:
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
                    state = STATE_IDENTIFIER;
                else {

                    result = check_identifier_for_kw(context->scanned_string);
                    if (result == IS_IDENTIFIER) {
                        token.type = IDENTIFIER;
                        result = add_identifier(data, context->scanned_string->chars);
                        if (result == -1) {
                            token.type = ERROR;
                            token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "identifier");

                            return token;
                        }

                        token.attribute_str_idx = result;

                    } else if (result == IS_BOOLEAN_VALUE_TRUE) {
                        token.type = BOOLEAN;
                        token.attribute_bool = true;
                    } else if (result == IS_BOOLEAN_VALUE_FALSE) {
                        token.type = BOOLEAN;
                        token.attribute_bool = false;
                    } else if (result == -2) {
                        token.type = ERROR;
                        token.attribute_error = print_error(CORRUPTED_MEMORY, state);
                        return token;
                    } else {
                        token.type = KEYWORD;
                        token.attribute_keyword = result;
                    }

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            // NUMBERS - INTEGER, DOUBLE
            case STATE_INTEGER:
                if (c == '_')
                    state = STATE_INTEGER_UNDERSCORE;
                else if (c >= '0' && c <= '9')
                    state = STATE_INTEGER;
                else if (c == 'e' || c == 'E')
                    state = STATE_DOUBLE_EXPONENT;
                else if (c == '.')
                    state = STATE_DECIMAL_POINT;
                else {
                    long value = strtol(context->scanned_string->chars, &end_ptr, 10);

                    if (context->scanned_string->chars == end_ptr || *end_ptr != '\0') {
                        token.type = ERROR;
                        token.attribute_error = print_error(NUMBER_CONVERT_ERROR, state);
                        return token;
                    }

                    token.type = INTEGER;
                    token.attribute_int = value;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_DOT:
                if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_DECIMAL_NUMBER;
                else {
                    token.type = DOT;
                    token.attribute_none = NULL;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_ZERO:
                if (c == 0)
                    state = STATE_ZERO;
                else if (c == '.')
                    state = STATE_DECIMAL_POINT;
                else if (c >= '1' && c <= '9')
                    state = STATE_DOUBLE_WHOLE_NUMBER;
                else if (c == '_')
                    state = STATE_DOUBLE_WHOLE_NUMBER_UNDERSCORE;
                else {
                    token.type = INTEGER;
                    token.attribute_int = 0;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_INTEGER_UNDERSCORE:

                scanned_string_remove_last(context->scanned_string); // remove _

                if (c >= '0' && c <= '9')
                    state = STATE_INTEGER;
                else if (c == '_')
                    state = STATE_INTEGER_UNDERSCORE;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_NUM_INVALID_UNDERSCORE, context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_DECIMAL_POINT:
                if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_DECIMAL_NUMBER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_DOUBLE_INVALID_DECIMAL_POINT, context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_DOUBLE_WHOLE_NUMBER:
                if (c == '.')
                    state = STATE_DECIMAL_POINT;
                else if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_WHOLE_NUMBER;
                else if (c == '_')
                    state = STATE_DOUBLE_WHOLE_NUMBER_UNDERSCORE;
                else if (c == 'e' || c == 'E')
                    state = STATE_DOUBLE_EXPONENT;
                else {
                    token.type = ERROR;
                    long valid_alternative = strtol(context->scanned_string->chars, &end_ptr, 10);
                    token.attribute_error = print_error(SCANNER_INT_LEADING_ZERO, context->scanned_string->chars, valid_alternative);
                    return token;
                }

                break;

            case STATE_DOUBLE_WHOLE_NUMBER_UNDERSCORE:

                scanned_string_remove_last(context->scanned_string); // remove _

                if (c == '_')
                    state = STATE_DOUBLE_WHOLE_NUMBER_UNDERSCORE;
                else if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_WHOLE_NUMBER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_NUM_INVALID_UNDERSCORE, context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_DOUBLE_DECIMAL_NUMBER:
                if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_DECIMAL_NUMBER;
                else if (c == 'e' || c == 'E')
                    state = STATE_DOUBLE_EXPONENT;
                else if (c == 'd' || c == 'D')
                    state = STATE_DOUBLE_END_D;
                else if (c == '_')
                    state = STATE_DOUBLE_DECIMAL_NUMBER_UNDERSCORE;
                else {
                    double value = strtod(context->scanned_string->chars, &end_ptr);

                    if (context->scanned_string->chars == end_ptr || *end_ptr != '\0') {
                        token.type = ERROR;
                        token.attribute_error = print_error(NUMBER_CONVERT_ERROR, state);
                        return token;
                    }

                    token.type = DOUBLE;
                    token.attribute_double = value;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_DOUBLE_DECIMAL_NUMBER_UNDERSCORE:

                scanned_string_remove_last(context->scanned_string); // remove _

                if (c == '_')
                    state = STATE_DOUBLE_DECIMAL_NUMBER_UNDERSCORE;
                else if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_DECIMAL_NUMBER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_NUM_INVALID_UNDERSCORE, context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_DOUBLE_END_D:
                scanned_string_remove_last(context->scanned_string); // remove d/D

                double value = strtod(context->scanned_string->chars, &end_ptr);

                if (context->scanned_string->chars == end_ptr || *end_ptr != '\0') {
                    token.type = ERROR;
                    token.attribute_error = print_error(NUMBER_CONVERT_ERROR, state);
                    return token;
                }

                token.type = DOUBLE;
                token.attribute_double = value;

                RETURN_TOKEN_AND_SET_READ_BUFFER

                break;

            case STATE_DOUBLE_EXPONENT:
                if (c == '+' || c == '-')
                    state = STATE_DOUBLE_EXPONENT_SIGN;
                else if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_EXPONENT_NUMBER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_DOUBLE_MISSING_EXPONENT, context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_DOUBLE_EXPONENT_SIGN:
                if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_EXPONENT_NUMBER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_DOUBLE_MISSING_EXPONENT, context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_DOUBLE_EXPONENT_NUMBER:
                if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_EXPONENT_NUMBER;
                else if (c == '_')
                    state = STATE_DOUBLE_EXPONENT_NUMBER_UNDERSCORE;
                else {
                    double double_value = strtod(context->scanned_string->chars, &end_ptr);

                    if (context->scanned_string->chars == end_ptr || *end_ptr != '\0') {
                        token.type = ERROR;
                        token.attribute_error = print_error(NUMBER_CONVERT_ERROR, state);
                        return token;
                    }

                    token.type = DOUBLE;
                    token.attribute_double = double_value;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_DOUBLE_EXPONENT_NUMBER_UNDERSCORE:

                scanned_string_remove_last(context->scanned_string); // remove _

                if (c == '_')
                    state = STATE_DOUBLE_DECIMAL_NUMBER_UNDERSCORE;
                else if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_EXPONENT_NUMBER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_NUM_INVALID_UNDERSCORE, context->scanned_string->chars);
                    return token;
                }

                break;

            // COMPARISON
            case STATE_LESSER_GREATER:

                if (c == '=')
                    state = STATE_COMPARE;
                else {
                    token.type = COMPARE;
                    char op = scanned_string_top(context->scanned_string);
                    if (op == '<')
                        token.attribute_compare_type = LESS;
                    else if (op == '>')
                        token.attribute_compare_type = GREATER;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_ASSIGNMENT:

                if (c == '=')
                    state = STATE_COMPARE;
                else {
                    token.type = ASSIGNMENT;
                    RETURN_TOKEN_AND_SET_READ_BUFFER;
                }

                break;

            case STATE_COMPARE:

                token.type = COMPARE;
                if (strcmp(context->scanned_string->chars, "==") == 0) {
                    token.attribute_compare_type = EQUAL;
                } else if (strcmp(context->scanned_string->chars, "!=") == 0) {
                    token.attribute_compare_type = NOT_EQUAL;
                } else if (strcmp(context->scanned_string->chars, ">=") == 0) {
                    token.attribute_compare_type = GREATER_EQUAL;
                } else if (strcmp(context->scanned_string->chars, "<=") == 0) {
                    token.attribute_compare_type = LESS_EQUAL;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(CORRUPTED_MEMORY, state);
                    return token;
                }

                RETURN_TOKEN_AND_SET_READ_BUFFER;
                break;

            // BRACKETS
            case STATE_LEFT_BRACKET:
                token.type = LEFT_BRACKET;
                token.attribute_none = NULL;

                RETURN_TOKEN_AND_SET_READ_BUFFER

            case STATE_RIGHT_BRACKET:
                token.type = RIGHT_BRACKET;
                token.attribute_none = NULL;

                RETURN_TOKEN_AND_SET_READ_BUFFER

            case STATE_RIGHT_CURLY_BRACKET:
                context->interpolation = context->master_interpolated_string; // restore the type of string interpolation
                scanned_string_remove_last(context->scanned_string); // remove }
                state = STATE_START;
                break;

            // DECLARATION - COLON, COMMA, AT
            case STATE_COLON:

                token.type = DECLARATION_SIGN;
                token.attribute_none = NULL;

                RETURN_TOKEN_AND_SET_READ_BUFFER
                break;

            case STATE_COMMA:

                token.type = COMMA;
                token.attribute_none = NULL;

                RETURN_TOKEN_AND_SET_READ_BUFFER
                break;

            case STATE_AT_SYMBOL:
                if (c >= 'a' && c <= 'z')
                    state = STATE_MAIN_IDENTIFIER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SYNTAX_INCORRECT_USAGE_OF_AT_SYMBOL);

                    return token;
                }

                break;

            case STATE_MAIN_IDENTIFIER:
                if (c >= 'a' && c <= 'z')
                    state = STATE_MAIN_IDENTIFIER;
                else {
                    token.type = MAIN_IDENTIFIER;

                    result = add_identifier(data, context->scanned_string->chars);
                    if (result == -1) {
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "main identifier");

                        return token;
                    }

                    token.attribute_str_idx = result;
                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_LINE_FEED:

                if (c == '\n') {
                    state = STATE_LINE_FEED;
                } else if (c == ' ') {
                    clear_scanned_string(context->scanned_string);
                    state = STATE_INDENT;
                } else if (c == EOF) {
                    token.type = END_OF_FILE;
                    token.attribute_none = NULL;
                    return token;
                } else {
                    token.type = LINE_FEED;
                    token.attribute_none = NULL;
                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_INDENT:

                if (c == ' ') {
                    state = STATE_INDENT;
                } else if (c == '\n' || c == EOF) {
                    state = STATE_LINE_FEED;
                } else {
                    token.type = INDENT;
                    token.attribute_int = context->scanned_string->len;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }
                break;

            // COMMENT
            case STATE_COMMENT_START:

                if (c == '\n' || c == EOF) {
                    state = STATE_LINE_FEED;
                } else {
                    state = STATE_COMMENT_START;
                }
                break;

            case STATE_BLOCK_COMMENT_START:

                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_BLOCK_COMMENT, "end of file", context->scanned_string->chars);
                    return token;
                }
                if (c == '/') {
                    state = STATE_ANOTHER_BLOCK_COMMENT_START;
                } else if (c == '\n') {
                    context->lf_inside_block_comment = true;
                    state = STATE_BLOCK_COMMENT_START;
                } else if (c == '*') {
                    state = STATE_ANOTHER_BLOCK_COMMENT_END;
                } else {
                    state = STATE_BLOCK_COMMENT_START;
                }
                break;

            case STATE_ANOTHER_BLOCK_COMMENT_START:

                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_BLOCK_COMMENT, "end of file", context->scanned_string->chars);
                    return token;
                }
                if (c == '*') {
                    context->block_comment_counter += 1;
                    state = STATE_BLOCK_COMMENT_START;
                } else {
                    state = STATE_BLOCK_COMMENT_START;
                }
                break;

            case STATE_ANOTHER_BLOCK_COMMENT_END:

                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_BLOCK_COMMENT, "end of file", context->scanned_string->chars);
                    return token;
                }
                if (c == '/' && context->block_comment_counter != 0) {
                    context->block_comment_counter -= 1;
                    state = STATE_BLOCK_COMMENT_START;
                } else if (c == '/' && context->block_comment_counter == 0 && context->lf_inside_block_comment == false) {
                    state = STATE_BLOCK_COMMENT_END_WITHOUT_LF;
                } else if (c == '/' && context->block_comment_counter == 0 && context->lf_inside_block_comment == true) {
                    state = STATE_BLOCK_COMMENT_END_WITH_LF;
                } else {
                    state = STATE_BLOCK_COMMENT_START;
                }
                break;

            case STATE_BLOCK_COMMENT_END_WITHOUT_LF:
                clear_scanned_string(context->scanned_string);
                state = STATE_START;
                continue;

            case STATE_BLOCK_COMMENT_END_WITH_LF:
                // between block comment and new line character can only be whitespace characters
                if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r')
                    state = STATE_BLOCK_COMMENT_END_WITH_LF;
                else if (c == '\n') {
                    state = STATE_LINE_FEED;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_BLOCK_COMMENT_FOLLOWED_BY_CHARS, context->scanned_string->chars);
                    return token;
                }

                break;

            default:
                token.attribute_error = print_error(SCANNER_NO_TRANSITION, state, c);
                return token;
        }

        // put character into the buffer
        result = scanned_string_append(context->scanned_string, c);

        if (result == 1) {
            // memory allocation fail
            token.type = ERROR;
            token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, "scanned_string");
            return token;
        }

        context->beginning_of_file = false;
    }
}