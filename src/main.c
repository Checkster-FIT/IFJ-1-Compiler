/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    main.c
 * @brief   The entry point for the compiler program
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "generator.h"
#include "parser.h"
#include "shared_data.h"
#include "symtable.h"

int main()
{
    SharedData data;
    init_shared_data(&data);

    Symtable* symtable = create_symtable();
    if (symtable == NULL)
        return print_error(MEMORY_ALLOCATION_ERROR, 0);

    ASTNode ast = create_ast();

    const int exit_code = run_parser(&ast, symtable, &data);

    run_generator(&ast, symtable, &data);

    destroy_shared_data(&data);
    return exit_code;
}