/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    ast.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "ast.h"
#include <stddef.h>
#include <stdlib.h>

void init_ast(AbstractSyntaxTree* ast)
{
    ast->extensions.first = NULL;
    ast->extensions.active = NULL;

    ast->functions.first = NULL;
    ast->functions.active = NULL;
}

void destroy_ast(AbstractSyntaxTree* ast)
{
    ast->extensions.first = NULL;
    ast->extensions.active = NULL;

    ast->functions.first = NULL;
    ast->functions.active = NULL;
}

ASTNode* add_function(AbstractSyntaxTree* ast, char* identifier)
{
    ASTNode* function = malloc(sizeof(ASTNode));

    if (function == NULL)
        return NULL;

    function->identifier = identifier;

    if (ast->functions.active == NULL)
        ast->functions.first = function;
    else
        ast->functions.active->next = function;

    ast->functions.active = function;
    return function;
}
