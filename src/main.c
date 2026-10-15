/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    main.c
 * @brief   The entry point for the compiler program
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "parser.h"
#include "scanner.h"

int main()
{
    // Create the scanner context for variables persisting token reading
    ScannerContext scanner_context;
    init_scanner(&scanner_context);

    const int status = run_parser();

    return status;
}