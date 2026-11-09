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
            .extensions = {
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
        case AST_EXTENSION:
            free_node_list(&node->extension.functions);
            free_ast_nodes(node->extension.parameter);
            free_symtable(node->extension.symtable);
            break;
        case AST_PARAMETER:
            break;
        case AST_VAR_DEF:
            free_ast_nodes(node->vardef.expression);
            break;
        case AST_VAR_ASSIGN:;
            free_ast_nodes(node->assignment.expression);
            break;
        case AST_EXPRESSION:
            free_ast_nodes(node->expression.left);
            free_ast_nodes(node->expression.right);
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

ASTNode* ast_add_extension(ASTNode* ast, Symtable* symtable)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_EXTENSION,
        .next = NULL,
        .extension = {
            .functions = { },
            .parameter = NULL,
            .symtable = symtable,
        },
    };

    ast_list_add(&ast->program.extensions, node);
    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_ext_function(ASTNode* ast, const char* identifier, Symtable* symtable)
{
    // TODO: Merge?

    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_FUNCTION,
        .next = NULL,
        .function = {
            .identifier = identifier,
            .statements = { },
            .parameters = { },
            .symtable = symtable },
    };

    ast_list_add(&ast->program.extensions.last->extension.functions, node);
    ast->program.recent_node = node;
    ast->program.last_function = node;
    return node;
}

ASTNode* ast_add_function(ASTNode* ast, const char* identifier, Symtable* symtable)
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
            .parameters = { },
            .symtable = symtable },
    };

    ast_list_add(&ast->program.functions, node);
    ast->program.recent_node = node;
    ast->program.last_function = node;
    return node;
}

ASTNode* ast_add_parameter(ASTNode* ast, const char* identifier)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_PARAMETER,
        .next = NULL,
        .parameter = {
            .identifier = identifier,
        },
    };

    ASTFunction* function = &ast->program.last_function->function;
    ast_list_add(&function->parameters, node);

    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_ext_parameter(ASTNode* ast, const char* identifier)
{
    // TODO: Merge?

    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_PARAMETER,
        .next = NULL,
        .parameter = {
            .identifier = identifier,
        },
    };

    ASTExtension* extension = &ast->program.extensions.last->extension;
    extension->parameter = node;

    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_vardef(ASTNode* ast, const char* identifier)
{
    ASTNode* node = malloc(sizeof(ASTNode));

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

    ast_list_add(&ast->program.last_function->function.statements, node);
    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_assignment(ASTNode* ast, const char* identifier)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_VAR_ASSIGN,
        .next = NULL,
        .assignment = {
            .target = identifier,
            .expression = NULL,
        },
    };

    ast_list_add(&ast->program.last_function->function.statements, node);
    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_expression(ASTNode* ast, ASTNode* expression)
{
    ast_list_add(&ast->program.last_function->function.statements, expression);
    ast->program.recent_node = expression;
    return expression;
}

ASTNode* ast_new_expression(const ASTExpression expression)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_EXPRESSION,
        .next = NULL,
        .expression = {
            .left = expression.left,
            .right = expression.right,
        }
    };

    return node;
}

Symtable* create_local_symtable(ASTNode* ast)
{
    // TODO: Unusued?
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
