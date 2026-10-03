/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    main.c
 * @brief   The entry point for the compiler program
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "parser.h"

int main(const int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    const int status = run_parser();

    return status;
}