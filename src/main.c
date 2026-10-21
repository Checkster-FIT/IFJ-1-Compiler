/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    main.c
 * @brief   The entry point for the compiler program
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "parser.h"
#include "shared_data.h"

int main()
{
    SharedData data;
    init_shared_data(&data);

    const int exit_code = run_parser(&data);

    destroy_shared_data(&data);
    return exit_code;
}