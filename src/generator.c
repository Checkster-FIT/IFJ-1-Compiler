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

    int* function_idx = &ast->program.functions.active->function.identifier_idx;
    SymtableFunction* function = (SymtableFunction*)search(symtable, function_idx);

    print_ast(ast, data);
    return 0;
}
