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
    // Create the scanner context for variables persisting token reading
    ScannerContext scanner;
    init_scanner(&scanner);

    Token token;
    token.type = NONE;

    while (token.type != END_OF_FILE) {

        token = get_token(&scanner, stdin);

        if (token.type == ERROR) {
            destroy_scanner(&scanner);
            return token.attribute_error;
        }
    }

    destroy_scanner(&scanner);
    return 0;
}