/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator.c
 * @brief
 * @author
 */

#include "generator.h"
#include "ast.h"
#include "debug.h"
#include "shared_data.h"

#include <stdio.h>

int run_generator(ASTNode* ast, const Symtable* symtable, const SharedData* data)
{
    (void)data;
    (void)symtable;

    int* function_idx = &ast->program.functions.last->function.identifier_idx;
    const SymtableEntry* function = (SymtableEntry*)search(symtable, function_idx)->value;

    (void)function;

    print_ast(ast, symtable, data);
    return 0;
}
