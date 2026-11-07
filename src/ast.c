/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    ast.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "ast.h"
#include "symtable.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

ASTNode* new_ast()
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    Symtable* symtable = new_symtable();

    if (symtable == NULL) {
        free(node);
        return NULL;
    }

    *node = (ASTNode) {
        .type = AST_PROGRAM,
        .next = NULL,
        .program = {
            .functions = {
                .first = NULL,
                .active = NULL,
                .last = NULL,
            },
            .recent_node = NULL,
            .global_symtable = symtable,
        },
    };

    return node;
}

void free_ast_nodes(ASTNode* node)
{
    if (node == NULL)
        return;

    switch (node->type) {
        case AST_NONE:
            break;
        case AST_PROGRAM:
            free_node_list(&node->program.functions);
            free_symtable(node->program.global_symtable);
            break;
        case AST_FUNCTION:
            free_node_list(&node->function.statements);
            free_node_list(&node->function.parameters);
            free_symtable(node->function.symtable);
            break;
        case AST_PARAMETER:
            break;
        case AST_VAR_DEF:
            free_ast_nodes(node->vardef.expression);
            break;
        case AST_VAR_ASSIGN:
            free_ast_nodes(node->assignment.target);
            free_ast_nodes(node->assignment.expression);
            break;
        case AST_VAR:
            break;
    }

    // fprintf(stderr,"Freed: %i\n", node->type);
    free(node);
}

void free_node_list(ASTNodeList* list)
{
    while (list->active) {
        ASTNode* next = list->active->next;
        free_ast_nodes(list->active);
        list->active = next;
    }
}

// ========================================

ASTNode* ast_add_function(ASTNode* ast, const char* identifier)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_FUNCTION,
        .next = NULL,
        .function = {
            .identifier = identifier,
            .statements = {},
        },
    };

    ast_list_add(&ast->program.functions, node);
    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_parameter(ASTNode* ast, const char* identifier)
{
    ASTNode* node = malloc(sizeof(ASTNode));
    ASTFunction* function = &ast->program.functions.last->function;

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_PARAMETER,
        .next = NULL,
        .parameter = {
            .identifier = identifier,
        },
    };

    ast_list_add(&function->parameters, node);
    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_vardef(ASTNode* ast, const char* identifier)
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
    ast->program.recent_node = node;
    return node;
}

Symtable* create_local_symtable(ASTNode* ast)
{
    ast->program.functions.last->function.symtable = new_symtable();
    return ast->program.functions.last->function.symtable;
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
