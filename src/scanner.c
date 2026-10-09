/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    scanner.c
 * @brief
 * @author
 */

#include "scanner.h"

#include <stdio.h>

Token get_token()
{
    int state = STATE_START;

    while (1) {
        int c = getchar();

        switch (state) {

        case (STATE_START):

            if (c == 0) {
                Token token;
                token.attribute_int = 0;
            }
        }
    }
}