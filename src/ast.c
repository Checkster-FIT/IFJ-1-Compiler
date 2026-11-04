/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    ast.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "ast.h"
#include <stddef.h>
#include <stdlib.h>

ASTNode create_ast()
{
    return (ASTNode) {
        .type = AST_PROGRAM,
        .next = NULL,
        .program = {
            .functions = {
                .first = NULL,
                .active = NULL,
                .last = NULL,
            },
        },
    };
}

void destroy_ast(ASTNode* ast)
{
    // TODO: Add AST destructor
    ast->type = AST_NONE;
}

// ========================================

ASTNode* add_vardef(ASTNode* ast, char* identifier)
{
    ASTNode* node = malloc(sizeof(ASTNode));
    ASTFunction* function = &ast->program.functions.last->function;

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_VAR_DEF,
        .next = NULL,
        .vardef = {
            .identifier = identifier,
            .expression = NULL,
        },
    };

    ast_list_add(&function->statements, node);
    ast->program.recent = node;
    return node;
}

ASTNode* add_function(ASTNode* ast, char* identifier)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_FUNCTION,
        .next = NULL,
        .function = {
            .identifier = identifier,
            .statements = { },
            .is_main = false,
        },
    };

    ast_list_add(&ast->program.functions, node);
    ast->program.recent = node;
    return node;
}

// ========================================

void ast_list_add(ASTNodeList* list, ASTNode* node)
{
    if (list->first == NULL) {
        list->first = node;
        list->active = node;
        list->last = node;
    } else if (list->last != NULL) {
        list->last->next = node;
        list->last = node;
    }
}
