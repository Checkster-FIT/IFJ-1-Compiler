/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    main.c
 * @brief   The entry point for the compiler program
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "generator.h"
#include "parser.h"
#include "shared_strings.h"
#include "symtable.h"

int main()
{
    AllocatedStrings* strings = new_strings();
    if (strings == NULL)
        return EXIT_CODE_INTERNAL_ERROR;

    Symtable* symtable = new_symtable();
    if (symtable == NULL)
        // TODO: Free identifiers
        return EXIT_CODE_INTERNAL_ERROR;

    ASTNode* ast = new_ast();
    if (ast == NULL)
        // TODO: Free identifiers, symtable
        return EXIT_CODE_INTERNAL_ERROR;

    const int exit_code_parser = run_parser(ast, symtable, strings);
    const int exit_code_generator = run_generator(ast, symtable);

    (void)exit_code_generator;
    return exit_code_parser;
}