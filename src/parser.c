/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    parser.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "parser.h"
#include "scanner.h"
#include <stdio.h>

int run_parser()
{
    Token token;
    token.type = NONE;

    while (token.type != ERROR && token.type != END_OF_FILE) {
        token = get_token(stdin);
    }

    return 0;
}