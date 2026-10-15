/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    scanner.c
 * @brief   Implementation of get_token().
 * @author  Drahovska Andrea, Wyslych Vojtech
 */

#include "scanner.h"

#include <stdio.h>

int init_scanner(ScannerContext* scanner_context)
{
    scanner_context->beginning_of_file = true;
    scanner_context->inside_interpolated_string = false;
    return 0;
}

Token get_token(ScannerContext context, FILE* stream)
{
    State state = STATE_START;
    Token token;

    token.type = NONE;
    token.attribute_none = NULL;

    while (1) {
        const int c = fgetc(stream);

        switch (state) {

            /*TODO:
             * r_curly_bracket:interpolo=true
             * SOF && space -> STATE_INDENT
             * if interpolo == true -> STATE_STRING_INTERPOLATION
             * END_OF_FILE Token? (xhajduf00)
             * Default state? (xhajduf00)
             */
            case (STATE_START):

                if (c == '&')
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
                else if (c == 10)
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
                else { // invalid input
                    token.type = ERROR;
                    return token;
                }
        }
    }
}