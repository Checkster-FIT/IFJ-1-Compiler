/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator.c
 * @brief
 * @author
 */

#include "generator.h"
#include "ast.h"
#include "debug.h"
#include <stdio.h>

int run_generator(ASTNode* ast, const Symtable* symtable)
{
    (void)symtable;

    const char* func_identifier = ast->program.functions.last->function.identifier;
    const SymtableEntry* function = (SymtableEntry*)search(symtable, func_identifier)->value;

    (void)function;

    print_ast(ast, symtable);
    return 0;
}
