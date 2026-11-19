/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    scanner.c
 * @brief   Implementation of get_token().
 * @author  Drahovska Andrea (xdrahoa00), Wyslych Vojtech (xwyslyv00)
 */

#include "scanner.h"
#include "error.h"
#include "shared_strings.h"
#include "stack.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE (context->line)

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

int init_scanner(ScannerContext* scanner_context, AllocatedStrings* strings)
{
    DynamicString* scanned_string = init_dynamic_string();
    if (!scanned_string)
        return 1;

    Stack* indent_stack = init_stack();
    if (!indent_stack) {
        dispose_dynamic_string(scanned_string);
        return 1;
    }

    scanner_context->beginning_of_file = true;
    scanner_context->interpolation = NO_INTERPOLATION;
    scanner_context->master_interpolated_string = NO_INTERPOLATION;
    scanner_context->scanned_string = scanned_string;
    scanner_context->indent_stack = indent_stack;
    scanner_context->pending_indent = 0;
    scanner_context->number_of_pending_dedents = 0;
    scanner_context->scanned_char_buffer = '\0';
    scanner_context->read_buffer = false;
    scanner_context->scanning_variable_inside_string_interpolation = false;
    scanner_context->pending_eof = false;
    scanner_context->block_comment_counter = 0;
    scanner_context->pending_dot = false;

    scanner_context->strings = strings;
    scanner_context->line = 1;
    return 0;
}

void destroy_scanner(const ScannerContext* scanner_context)
{
    dispose_dynamic_string(scanner_context->scanned_string);
    dispose_stack(scanner_context->indent_stack);
}

int check_identifier_for_kw(DynamicString* string)
{
    if (!string)
        return -2;

    for (int i = 0; i < NUMBER_OF_KEYWORDS; i++) {
        if (strcmp(string->chars, keywords[i]) == 0)
            return i;
    }

    return IS_IDENTIFIER;
}

int check_indentation(ScannerContext* context, int current_indent, bool first_call)
{
    int const s_top = (int)(intptr_t)top(context->indent_stack);
    if (s_top == current_indent && first_call == true)
        return NO_INDENTATION;
    if (s_top == current_indent && first_call == false)
        return DEDENTATION;
    if (s_top < current_indent && first_call == true) {
        push(context->indent_stack, (void*)(intptr_t)current_indent);
        return INDENTATION;
    }
    if (s_top < current_indent && first_call == false) {
        return INVALID_INDENTATION;
    }

    pop(context->indent_stack);
    context->number_of_pending_dedents += 1;
    return check_indentation(context, current_indent, false);
}

/** TODO:
 *
 * TAB on empty line should not throw an error
 *
 * nested interpolation - simple string x multiline string
 *
 */
Token get_token(ScannerContext* context, FILE* stream)
{
    State state = STATE_START;
    Token token;

    token.type = ERROR; // default setting to ERROR
    token.attribute_error = 1; // lexical error exit code

    clear_dynamic_string(context->scanned_string);

    int indent_value = 0;

    // first check for pending dedent
    if (context->number_of_pending_dedents) {
        context->number_of_pending_dedents -= 1;
        token.type = DEDENT;
        return token;
    }

    if (context->pending_indent) {
        token.type = INDENT;
        token.attribute_int = context->pending_indent;
        context->pending_indent = 0;
        return token;
    }

    // then check for pending EOF
    // once the EOF is reached by scanner, EOF token will be returned on each following get_token() call
    if (context->pending_eof) {
        token.type = END_OF_FILE;
        return token;
    }

    if (context->pending_dot) {
        context->pending_dot = false;

        token.type = DOT;
        return token;
    }

    while (1) {
        char c;
        char* str;
        int status;
        char* end_ptr;

        if (context->read_buffer) {
            c = context->scanned_char_buffer;
        } else {
            c = (char)fgetc(stream);

            if (ferror(stream)) {
                token.type = ERROR;
                token.attribute_error = print_error(STREAM_READ_ERROR, LINE);
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
                else if (c == ';')
                    state = STATE_SEMICOLON;
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
                else if (c == 9) {
                    // tab
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_INDENTATION_USING_TAB, LINE);
                    return token;
                } else if (c == ' ' && context->beginning_of_file == true) {
                    indent_value = 1;
                    state = STATE_INDENT;
                } else if ((c == ' ' && context->beginning_of_file == false) || c == '\r')
                    continue;
                else if (c == EOF)
                    state = STATE_END_OF_FILE;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_CHARACTER, LINE, c);
                    return token;
                }

                break;

            // OPERATOR
            case STATE_AMPERSAND_1:

                if (c == '&')
                    state = STATE_OPERATOR;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_UNKNOWN_OPERATOR, LINE, context->scanned_string->chars,
                        "&&");
                    return token;
                }

                break;

            case STATE_OR_1:

                if (c == '|')
                    state = STATE_OPERATOR;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_UNKNOWN_OPERATOR, LINE, context->scanned_string->chars,
                        "||");
                    return token;
                }

                break;

            case STATE_SLASH:
                if (c == '/')
                    state = STATE_COMMENT;
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

                char a = dynamic_string_top(context->scanned_string);

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
                    token.attribute_error = print_error(CORRUPTED_MEMORY, LINE);
                    return token;
                }

                RETURN_TOKEN_AND_SET_READ_BUFFER

                // STRING

            case STATE_STRING_WHICH:

                // remove `"` from beginning of the scanned string since parser only needs the inside of the string
                dynamic_string_remove_last(context->scanned_string);

                if (c == '"')
                    state = STATE_EMPTY_STRING;
                else if (c == '\\')
                    state = STATE_STRING_ESCAPE;
                else if (c != '\n' && c != EOF && c != '\\' && c != '"')
                    state = STATE_STRING_BEGIN;
                else if (c == '\n') {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "a new line character",
                        context->scanned_string->chars);
                    return token;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
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
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "a new line character",
                        context->scanned_string->chars);
                    return token;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_STRING_ESCAPE:

                dynamic_string_remove_last(context->scanned_string); // remove '\'

                if (c == '\\' || c == '"') {
                    status = dynamic_string_append(context->scanned_string, c);
                    if (status == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
                        return token;
                    }
                    state = STATE_STRING_BEGIN;
                    break;
                }

                if (c == 'n') {
                    state = STATE_STRING_BEGIN;
                    status = dynamic_string_append(context->scanned_string, '\n');
                    if (status == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
                        return token;
                    }
                    continue; // ignore 'n'
                }

                if (c == 'r') {
                    state = STATE_STRING_BEGIN;
                    status = dynamic_string_append(context->scanned_string, '\r');
                    if (status == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
                        return token;
                    }
                    continue;
                }

                if (c == 't') {
                    state = STATE_STRING_BEGIN;
                    status = dynamic_string_append(context->scanned_string, '\t');
                    if (status == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
                        return token;
                    }
                    continue;
                }

                // no other escape sequence is allowed
                token.type = ERROR;
                token.attribute_error = print_error(SCANNER_INVALID_ESCAPE_SEQUENCE, LINE, c);
                return token;

            case STATE_STRING_END:

                dynamic_string_remove_last(context->scanned_string);
                token.type = STRING;

                str = add_string_copy(context->strings, context->scanned_string->chars);
                if (str == NULL) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned string");

                    return token;
                }

                token.attribute_string = str;

                RETURN_TOKEN_AND_SET_READ_BUFFER
            case STATE_EMPTY_STRING:

                if (c == '"') {
                    clear_dynamic_string(context->scanned_string);
                    state = STATE_STRING_MULTILINE_START;
                    // we dont want `"` to be saved into scanned string to we continue to the next char from the stream
                    // (another possible solution would be adding another state something like STATE_STRING_MULTILINE_VERY_START which would remove the '"')
                    continue;
                }

                str = add_string_copy(context->strings, "");
                if (str == NULL) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned string");

                    return token;
                }

                token.type = STRING;
                token.attribute_string = str;

                RETURN_TOKEN_AND_SET_READ_BUFFER
            case STATE_STRING_MULTILINE_START:

                if (c == '"')
                    state = STATE_STRING_MULTILINE_END_1;
                else if (c == '\n') {
                    context->line += 1;
                    state = STATE_STRING_MULTILINE_START;
                } else if (c != EOF && c != '"')
                    state = STATE_STRING_MULTILINE_START;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
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
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
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
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_STRING_MULTILINE_END_3:

                // remove the ending `"""` from the string
                dynamic_string_remove_last(context->scanned_string);
                dynamic_string_remove_last(context->scanned_string);
                dynamic_string_remove_last(context->scanned_string);

                token.type = MULTILINE_STRING;

                str = add_string_copy(context->strings, context->scanned_string->chars);
                if (str == NULL) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned multiline string");

                    return token;
                }

                token.attribute_string = str;

                RETURN_TOKEN_AND_SET_READ_BUFFER

                // INTERPOLATED STRING

            case STATE_CHARACTER_S:

                if (c == '"')
                    state = STATE_STRING_INTERPOLATION_WHICH_1;
                else if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                    state = STATE_IDENTIFIER;
                else {
                    token.type = IDENTIFIER;

                    str = add_string_copy(context->strings, context->scanned_string->chars);
                    if (token.attribute_string == NULL) {
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "identifier");
                        return token;
                    }
                    token.attribute_string = str;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_STRING_INTERPOLATION_WHICH_1:
                clear_dynamic_string(context->scanned_string); // remove s
                clear_dynamic_string(context->scanned_string); // remove "

                // can be overwritten by multiline interpolation
                // it is here to prevent code duplication as it would be needed to set in STATE_STRING_INTERPOLATION as well as in STATE_STRING_INTERPOLATION_DOLLAR
                // (interpolated string can start with $: s"$var")
                context->interpolation = SIMPLE_INTERPOLATED_STRING;
                context->master_interpolated_string = SIMPLE_INTERPOLATED_STRING;

                if (c == '"')
                    state = STATE_STRING_INTERPOLATION_WHICH_2;
                else if (c == '\n') {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "a new line character",
                        context->scanned_string->chars);
                    return token;
                } else if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
                    return token;
                } else if (c == '$')
                    state = STATE_STRING_INTERPOLATION_DOLLAR;
                else
                    state = STATE_STRING_INTERPOLATION;

                break;

            case STATE_STRING_INTERPOLATION_WHICH_2:
                dynamic_string_remove_last(context->scanned_string); // removes "

                if (c == '"') {
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                    continue; // don't save the last "
                }

                context->interpolation = NO_INTERPOLATION;
                context->master_interpolated_string = NO_INTERPOLATION;
                token.type = STRING;
                str = add_string_copy(context->strings, "");
                if (str == NULL) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "string interpolation");
                    return token;
                }
                token.attribute_string = str;

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
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
                    return token;
                } else if (c == '\n') {
                    context->line += 1;
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                } else
                    state = STATE_MULTILINE_STRING_INTERPOLATION;

                break;

            case STATE_MULTILINE_STRING_INTERPOLATION_ESCAPE:

                dynamic_string_remove_last(context->scanned_string); // remove '\'

                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
                    return token;
                }

                if (c == 'n') {
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                    status = dynamic_string_append(context->scanned_string, '\n');
                    if (status == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
                        return token;
                    }
                    continue; // ignore 'n'
                }

                if (c == 'r') {
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                    status = dynamic_string_append(context->scanned_string, '\r');
                    if (status == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
                        return token;
                    }
                    continue; // ignore 'r'
                }

                if (c == 't') {
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                    status = dynamic_string_append(context->scanned_string, '\t');
                    if (status == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
                        return token;
                    }
                    continue; // ignore 't'
                }

                state = STATE_MULTILINE_STRING_INTERPOLATION;

                break;

            case STATE_MULTILINE_STRING_INTERPOLATION_DOLLAR:

                dynamic_string_remove_last(context->scanned_string); // removes $

                if (c == '$')
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                else if (c == '{')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_EXPRESSION;
                else {
                    // pointer to the whole string before $
                    token.type = MULTILINE_STRING_INTERPOLATION;

                    str = add_string_copy(context->strings, context->scanned_string->chars);
                    if (str == NULL) {
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE,
                            "multiline string interpolation");

                        return token;
                    }

                    token.attribute_string = str;
                    context->scanning_variable_inside_string_interpolation = true;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_MULTILINE_STRING_INTERPOLATION_EXPRESSION:

                context->interpolation = NO_INTERPOLATION;

                dynamic_string_remove_last(context->scanned_string);
                // removes { (dollar is already removed in the previous state)

                token.type = MULTILINE_STRING_INTERPOLATION;
                // pointer to the whole string before {
                str = add_string_copy(context->strings, context->scanned_string->chars);
                if (str == NULL) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "multiline string interpolation");

                    return token;
                }

                token.attribute_string = str;

                RETURN_TOKEN_AND_SET_READ_BUFFER;

            case STATE_MULTILINE_STRING_INTERPOLATION_END_1:
                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
                    return token;
                } else if (c == '"')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_END_2;
                else if (c == '$')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_DOLLAR;
                else if (c == '\\')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_ESCAPE;
                else if (c == '\n') {
                    context->line += 1;
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                } else
                    state = STATE_MULTILINE_STRING_INTERPOLATION;

                break;

            case STATE_MULTILINE_STRING_INTERPOLATION_END_2:
                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
                    return token;
                } else if (c == '"')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_END_3;
                else if (c == '$')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_DOLLAR;
                else if (c == '\\')
                    state = STATE_MULTILINE_STRING_INTERPOLATION_ESCAPE;
                else if (c == '\n') {
                    context->line += 1;
                    state = STATE_MULTILINE_STRING_INTERPOLATION;
                } else
                    state = STATE_MULTILINE_STRING_INTERPOLATION;

                break;

            case STATE_MULTILINE_STRING_INTERPOLATION_END_3:
                context->interpolation = NO_INTERPOLATION;
                context->master_interpolated_string = NO_INTERPOLATION;

                // remove tailing """
                for (int i = 0; i < 3; i++) {
                    dynamic_string_remove_last(context->scanned_string);
                }

                token.type = MULTILINE_STRING_INTERPOLATION_END;

                str = add_string_copy(context->strings, context->scanned_string->chars);
                if (str == NULL) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "multiline string interpolation");

                    return token;
                }

                token.attribute_string = str;

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
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "a new line character",
                        context->scanned_string->chars);
                    return token;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_STRING_INTERPOLATION_DOLLAR:

                dynamic_string_remove_last(context->scanned_string); // removes $

                if (c == '$')
                    state = STATE_STRING_INTERPOLATION;
                else if (c == '{')
                    state = STATE_STRING_INTERPOLATION_EXPRESSION;
                else {
                    // pointer to the whole string before $
                    token.type = STRING_INTERPOLATION;

                    str = add_string_copy(context->strings, context->scanned_string->chars);
                    if (str == NULL) {
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "string interpolation");

                        return token;
                    }

                    token.attribute_string = str;
                    context->scanning_variable_inside_string_interpolation = true;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_STRING_INTERPOLATION_EXPRESSION:

                context->interpolation = NO_INTERPOLATION;

                dynamic_string_remove_last(context->scanned_string); // removes {

                token.type = STRING_INTERPOLATION;
                // pointer to the whole string before {
                str = add_string_copy(context->strings, context->scanned_string->chars);
                if (str == NULL) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "string interpolation");

                    return token;
                }

                token.attribute_string = str;

                RETURN_TOKEN_AND_SET_READ_BUFFER;

            case STATE_STRING_INTERPOLATION_END:

                context->interpolation = NO_INTERPOLATION;
                context->master_interpolated_string = NO_INTERPOLATION;

                dynamic_string_remove_last(context->scanned_string); // removes "

                token.type = STRING_INTERPOLATION_END;

                // pointer to the whole string before "
                str = add_string_copy(context->strings, context->scanned_string->chars);
                if (str == NULL) {
                    token.type = ERROR;
                    token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "string interpolation");

                    return token;
                }

                token.attribute_string = str;

                RETURN_TOKEN_AND_SET_READ_BUFFER;

            case STATE_STRING_INTERPOLATION_ESCAPE:

                dynamic_string_remove_last(context->scanned_string); // remove '\'

                if (c == 'n') {
                    state = STATE_STRING_INTERPOLATION;
                    status = dynamic_string_append(context->scanned_string, '\n');
                    if (status == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
                        return token;
                    }
                    continue; // ignore 'n'
                }

                if (c == 't') {
                    state = STATE_STRING_INTERPOLATION;
                    status = dynamic_string_append(context->scanned_string, '\t');
                    if (status == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
                        return token;
                    }
                    continue;
                }

                if (c == 'r') {
                    state = STATE_STRING_INTERPOLATION;
                    status = dynamic_string_append(context->scanned_string, '\r');
                    if (status == 1) {
                        // memory allocation fail
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
                        return token;
                    }
                    continue;
                }

                if (c == '\n') {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "a new line character",
                        context->scanned_string->chars);
                    return token;
                }

                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_STRING, LINE, "end of file",
                        context->scanned_string->chars);
                    return token;
                }

                state = STATE_STRING_INTERPOLATION;
                break;

            // IDENTIFIER / KEYWORD / BOOLEAN
            case STATE_UNDERSCORE_IDENTIFIER:

                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
                    state = STATE_IDENTIFIER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_IDENTIFIER, LINE, "_");
                    return token;
                }

                break;

            case STATE_IDENTIFIER:
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
                    state = STATE_IDENTIFIER;
                else {
                    status = check_identifier_for_kw(context->scanned_string);
                    if (status == IS_IDENTIFIER) {
                        token.type = IDENTIFIER;
                        str = add_string_copy(context->strings, context->scanned_string->chars);
                        if (str == NULL) {
                            token.type = ERROR;
                            token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "identifier");
                            return token;
                        }

                        token.attribute_string = str;
                    } else if (status == IS_BOOLEAN_VALUE_TRUE) {
                        token.type = BOOLEAN;
                        token.attribute_bool = true;
                    } else if (status == IS_BOOLEAN_VALUE_FALSE) {
                        token.type = BOOLEAN;
                        token.attribute_bool = false;
                    } else if (status == -2) {
                        token.type = ERROR;
                        token.attribute_error = print_error(CORRUPTED_MEMORY, LINE);
                        return token;
                    } else {
                        token.type = KEYWORD;
                        token.attribute_keyword = status;
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
                        token.attribute_error = print_error(NUMBER_CONVERT_ERROR, LINE, state);
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
                if (c == '0')
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

                dynamic_string_remove_last(context->scanned_string); // remove _

                if (c >= '0' && c <= '9')
                    state = STATE_INTEGER;
                else if (c == '_')
                    state = STATE_INTEGER_UNDERSCORE;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_NUM_INVALID_UNDERSCORE, LINE,
                        context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_DECIMAL_POINT:
                if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_DECIMAL_NUMBER;

                // dot notation (e.x. 12.toInt)
                else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                    dynamic_string_remove_last(context->scanned_string); // remove .

                    long value = strtol(context->scanned_string->chars, &end_ptr, 10);

                    if (context->scanned_string->chars == end_ptr || *end_ptr != '\0') {
                        token.type = ERROR;
                        token.attribute_error = print_error(NUMBER_CONVERT_ERROR, LINE, state);
                        return token;
                    }

                    token.type = INTEGER;
                    token.attribute_int = value;

                    context->pending_dot = true;
                    RETURN_TOKEN_AND_SET_READ_BUFFER // read this c again

                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_DOUBLE_INVALID_DECIMAL_POINT, LINE,
                        context->scanned_string->chars);
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
                    token.attribute_error = print_error(SCANNER_INT_LEADING_ZERO, LINE, context->scanned_string->chars,
                        valid_alternative);
                    return token;
                }

                break;

            case STATE_DOUBLE_WHOLE_NUMBER_UNDERSCORE:

                dynamic_string_remove_last(context->scanned_string); // remove _

                if (c == '_')
                    state = STATE_DOUBLE_WHOLE_NUMBER_UNDERSCORE;
                else if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_WHOLE_NUMBER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_NUM_INVALID_UNDERSCORE, LINE,
                        context->scanned_string->chars);
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
                        token.attribute_error = print_error(NUMBER_CONVERT_ERROR, LINE, state);
                        return token;
                    }

                    token.type = DOUBLE;
                    token.attribute_double = value;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_DOUBLE_DECIMAL_NUMBER_UNDERSCORE:

                dynamic_string_remove_last(context->scanned_string); // remove _

                if (c == '_')
                    state = STATE_DOUBLE_DECIMAL_NUMBER_UNDERSCORE;
                else if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_DECIMAL_NUMBER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_NUM_INVALID_UNDERSCORE, LINE,
                        context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_DOUBLE_END_D:
                dynamic_string_remove_last(context->scanned_string); // remove d/D

                double value = strtod(context->scanned_string->chars, &end_ptr);

                if (context->scanned_string->chars == end_ptr || *end_ptr != '\0') {
                    token.type = ERROR;
                    token.attribute_error = print_error(NUMBER_CONVERT_ERROR, LINE, state);
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
                    token.attribute_error = print_error(SCANNER_DOUBLE_MISSING_EXPONENT, LINE,
                        context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_DOUBLE_EXPONENT_SIGN:
                if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_EXPONENT_NUMBER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_DOUBLE_MISSING_EXPONENT, LINE,
                        context->scanned_string->chars);
                    return token;
                }

                break;

            case STATE_DOUBLE_EXPONENT_NUMBER:
                if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_EXPONENT_NUMBER;
                else if (c == '_')
                    state = STATE_DOUBLE_EXPONENT_NUMBER_UNDERSCORE;
                else if (c == 'd' || c == 'D')
                    state = STATE_DOUBLE_END_D;
                else {
                    double double_value = strtod(context->scanned_string->chars, &end_ptr);

                    if (context->scanned_string->chars == end_ptr || *end_ptr != '\0') {
                        token.type = ERROR;
                        token.attribute_error = print_error(NUMBER_CONVERT_ERROR, LINE, state);
                        return token;
                    }

                    token.type = DOUBLE;
                    token.attribute_double = double_value;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            case STATE_DOUBLE_EXPONENT_NUMBER_UNDERSCORE:

                dynamic_string_remove_last(context->scanned_string); // remove _

                if (c == '_')
                    state = STATE_DOUBLE_EXPONENT_NUMBER_UNDERSCORE;
                else if (c >= '0' && c <= '9')
                    state = STATE_DOUBLE_EXPONENT_NUMBER;
                else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_NUM_INVALID_UNDERSCORE, LINE,
                        context->scanned_string->chars);
                    return token;
                }

                break;

            // COMPARISON
            case STATE_LESSER_GREATER:

                if (c == '=')
                    state = STATE_COMPARE;
                else {
                    token.type = COMPARE;
                    char op = dynamic_string_top(context->scanned_string);
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
                    token.attribute_error = print_error(CORRUPTED_MEMORY, LINE);
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
                context->interpolation = context->master_interpolated_string;
                // restore the type of string interpolation
                dynamic_string_remove_last(context->scanned_string); // remove }
                state = STATE_START;

                context->read_buffer = true; // following character is already consumed and needs to be handled properly
                continue; // don't write the char into scanned_string

            // DECLARATION - COLON, COMMA, AT
            case STATE_COLON:

                token.type = DECLARATION_SIGN;
                token.attribute_none = NULL;

                RETURN_TOKEN_AND_SET_READ_BUFFER
                break;

            case STATE_SEMICOLON:
                token.type = SEMICOLON;
                token.attribute_none = NULL;

                RETURN_TOKEN_AND_SET_READ_BUFFER
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
                    token.attribute_error = print_error(SYNTAX_INCORRECT_USAGE_OF_AT_SYMBOL, LINE);

                    return token;
                }

                break;

            case STATE_MAIN_IDENTIFIER:
                if (c >= 'a' && c <= 'z')
                    state = STATE_MAIN_IDENTIFIER;
                else {
                    token.type = MAIN_IDENTIFIER;

                    str = add_string_copy(context->strings, context->scanned_string->chars);
                    if (str == NULL) {
                        token.type = ERROR;
                        token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "main identifier");

                        return token;
                    }

                    token.attribute_string = str;
                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }

                break;

            // STATE_LINE_FEED serves as a kind of pipe into STATE_INDENT which will handle
            // indentation as well as line feed token
            case STATE_LINE_FEED:

                context->line += 1;

                if (c == '\n') {
                    state = STATE_LINE_FEED;
                    break;
                }
                if (c == EOF) {
                    state = STATE_END_OF_FILE;
                    break;
                }
                if (c == '/') {
                    state = STATE_SLASH;
                    break;
                }
                if (c == ' ') {
                    indent_value = 1;
                    state = STATE_INDENT;
                    break;
                }
                if (c == 9) {
                    // tab
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_INDENTATION_USING_TAB, LINE);
                    return token;
                }
                if (c == '\r') {
                    state = STATE_LINE_FEED;
                    break;
                }

                // no whitespace on new line
                indent_value = 0;
                state = STATE_INDENT;
                context->read_buffer = true; // keep the read char as STATE_INDENT needs to read it again
                break;

            // STATE_INDENT is only accessible after line feed (or BOF)
            // read spaces as long as possible, then handle indent value
            //      return line feed token and if needed,
            //      INDENT/DEDENT token will be returned on the next get_token() call
            case STATE_INDENT:

                if (c == ' ') {
                    indent_value++;
                    state = STATE_INDENT;
                } else if (c == '\n' || c == EOF)
                    state = STATE_LINE_FEED;
                else if (c == '/')
                    state = STATE_SLASH;
                else if (c == '\r') {
                    state = STATE_INDENT;
                    break;
                } else {
                    status = check_indentation(context, indent_value, true);

                    if (status == INVALID_INDENTATION) {
                        token.type = ERROR;
                        token.attribute_error = print_error(SCANNER_INVALID_INDENTATION, LINE);
                        return token;
                    }

                    if (status == INDENTATION)
                        context->pending_indent = indent_value; // return INDENT token on the next get_token() call

                    // dedentation is already handled by check_indentation()

                    // no indentation:
                    token.type = LINE_FEED;

                    RETURN_TOKEN_AND_SET_READ_BUFFER
                }
                break;

            // Correct handling of EOF is needed for parser.
            // When EOF is found by scanner, set context's parameters: pending dedents and pending EOF.
            case STATE_END_OF_FILE:
                context->pending_eof = true;

                while (context->indent_stack->top != NULL) {
                    context->number_of_pending_dedents += 1;
                    pop(context->indent_stack);
                }

                token.type = LINE_FEED;
                return token;

            // COMMENT
            case STATE_COMMENT:

                if (c == '\n' || c == EOF)
                    state = STATE_LINE_FEED;
                else {
                    state = STATE_COMMENT;
                }
                break;

            case STATE_BLOCK_COMMENT_START:

                if (c == EOF) {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_INVALID_BLOCK_COMMENT, LINE, "end of file",
                        context->scanned_string->chars);
                    return token;
                }
                if (c == '/') {
                    state = STATE_ANOTHER_BLOCK_COMMENT_START;
                } else if (c == '\n') {
                    context->line += 1;
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
                    token.attribute_error = print_error(SCANNER_INVALID_BLOCK_COMMENT, LINE, "end of file",
                        context->scanned_string->chars);
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
                    token.attribute_error = print_error(SCANNER_INVALID_BLOCK_COMMENT, LINE, "end of file",
                        context->scanned_string->chars);
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
                clear_dynamic_string(context->scanned_string);
                state = STATE_START;

                context->read_buffer = true;
                continue;

            case STATE_BLOCK_COMMENT_END_WITH_LF:
                // between block comment and new line character can only be whitespace characters
                if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r')
                    state = STATE_BLOCK_COMMENT_END_WITH_LF;
                else if (c == '\n') {
                    state = STATE_LINE_FEED;
                } else {
                    token.type = ERROR;
                    token.attribute_error = print_error(SCANNER_BLOCK_COMMENT_FOLLOWED_BY_CHARS, LINE,
                        context->scanned_string->chars);
                    return token;
                }

                break;

            default:
                token.attribute_error = print_error(SCANNER_NO_TRANSITION, LINE, state, c);
                return token;
        }

        // put character into the buffer
        status = dynamic_string_append(context->scanned_string, c);

        if (status == 1) {
            // memory allocation fail
            token.type = ERROR;
            token.attribute_error = print_error(MEMORY_ALLOCATION_ERROR, LINE, "scanned_string");
            return token;
        }

        context->beginning_of_file = false;
    }
}