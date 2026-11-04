/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator.c
 * @brief
 * @author
 */

#include "generator.h"
#include "ast.h"
#include "shared_data.h"
#include "debug.h"

#include <stdio.h>

int run_generator(ASTNode* ast, const SharedData* data)
{
    (void) data;
    print_ast(ast, data);
    return 0;
}
