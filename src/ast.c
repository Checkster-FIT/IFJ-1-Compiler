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

ASTNode* ast_add_function(ASTNode* ast, const int identifier_idx, const bool main)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_FUNCTION,
        .next = NULL,
        .function = {
            .identifier_idx = identifier_idx,
            .statements = { },
            .is_main = main,
        },
    };

    ast_list_add(&ast->program.functions, node);
    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_parameter(ASTNode* ast, const int identifier_idx)
{
    ASTNode* node = malloc(sizeof(ASTNode));
    ASTFunction* function = &ast->program.functions.last->function;

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_PARAMETER,
        .next = NULL,
        .parameter = {
            .identifier_idx = identifier_idx,
        },
    };

    ast_list_add(&function->parameters, node);
    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_vardef(ASTNode* ast, const int identifier_idx)
{
    ASTNode* node = malloc(sizeof(ASTNode));
    ASTFunction* function = &ast->program.functions.last->function;

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_VAR_DEF,
        .next = NULL,
        .vardef = {
            .identifier_idx = identifier_idx,
            .expression = NULL,
        },
    };

    ast_list_add(&function->statements, node);
    ast->program.recent_node = node;
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
