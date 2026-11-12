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
        .parent = NULL,
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
            .block_stack = init_stack(),
        },
    };

    if (node->program.block_stack == NULL) {
        free(node);
        free(symtable);
        return NULL;
    }

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
            free_node_list(&node->program.extensions);
            free_symtable(node->program.global_symtable);
            dispose_stack(node->program.block_stack);
            break;
        case AST_FUNCTION:
            free_node_list(&node->function.statements);
            free_node_list(&node->function.parameter_groups);
            free_symtable(node->function.symtable);
            break;
        case AST_EXTENSION:
            free_node_list(&node->extension.functions);
            free_node_list(&node->extension.parameter_groups);
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
        case AST_RETURN:;
            free_ast_nodes(node->return_st.expression);
            break;
        case AST_EXPRESSION:
            free_ast_nodes(node->expression.left);
            free_ast_nodes(node->expression.right);
            break;
        case AST_LITERAL:
            break;
        case AST_TERM:
            free_node_list(&node->term.parameters);
            break;
        case AST_WHILE_DO:
            free_ast_nodes(node->while_do.condition);
            free_node_list(&node->while_do.statements);
            free_symtable(node->while_do.symtable);
            break;
        case AST_INTERPOLATED_STRING:
            free_node_list(&node->interpolated_string.expressions);
            break;
        case AST_PARAMETER_GROUP:
            free_node_list(&node->parameter_group.parameters);
            break;
    }

    // fprintf(stderr,"Freed: %i\n", node->type);
    free(node);
}

void free_node_list(ASTNodeList* list)
{
    list->active = list->first;

    while (list->active) {
        ASTNode* next = list->active->next;
        free_ast_nodes(list->active);
        list->active = next;
    }
}

void ast_list_add(ASTNodeList* list, ASTNode* node)
{
    if (list == NULL)
        return;

    if (list->first == NULL) {
        list->first = node;
        list->active = node;
        list->last = node;
    } else if (list->last != NULL) {
        list->last->next = node;
        list->last = node;
    }
}

ASTNodeList* get_top_statement_block(const ASTNode* ast)
{
    ASTNode* node = top(ast->program.block_stack);

    if (node == NULL)
        return NULL;

    if (node->type == AST_FUNCTION)
        return &node->function.statements;
    if (node->type == AST_WHILE_DO)
        return &node->while_do.statements;

    return NULL;
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
        .parent = ast,
        .extension = {
            .functions = { },
            .parameter_groups = { },
            .symtable = symtable,
        },
    };

    ast_list_add(&node->parent->program.extensions, node);

    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_function(ASTNode* ast, const char* identifier, Symtable* symtable, const ASTTarget target)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_FUNCTION,
        .next = NULL,
        .parent = NULL,
        .function = {
            .identifier = identifier,
            .statements = { },
            .parameter_groups = { },
            .symtable = symtable,
        },
    };

    if (target == AST_TARGET_FUNCTION) {
        node->parent = ast;
        ast_list_add(&node->parent->program.functions, node);
    }
    else if (target == AST_TARGET_EXTENSION) {
        node->parent = ast->program.extensions.last;
        ast_list_add(&node->parent->extension.functions, node);
    }

    push(ast->program.block_stack, node);

    ast->program.recent_function = node;
    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_parameter_group(ASTNode* ast, const ASTTarget target)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_PARAMETER_GROUP,
        .next = NULL,
        .parent = NULL,
        .parameter_group = {
            .parameters = { },
        },
    };

    if (target == AST_TARGET_FUNCTION) {
        node->parent = ast->program.recent_function;
        ast_list_add(&node->parent->function.parameter_groups, node);
    }
    else if (target == AST_TARGET_EXTENSION) {
        node->parent = ast->program.extensions.last;
        ast_list_add(&node->parent->extension.parameter_groups, node);
    }

    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_parameter(ASTNode* ast, const char* identifier, const ASTTarget target)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_PARAMETER,
        .next = NULL,
        .parent = NULL,
        .parameter = {
            .identifier = identifier,
        },
    };

    if (target == AST_TARGET_FUNCTION) {
        node->parent = ast->program.recent_function->function.parameter_groups.last;
        ast_list_add(&node->parent->parameter_group.parameters, node);
    }
    else if (target == AST_TARGET_EXTENSION) {
        node->parent = ast->program.extensions.last->extension.parameter_groups.last;
        ast_list_add(&node->parent->parameter_group.parameters, node);
    }

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

    node->parent = top(ast->program.block_stack);
    ast_list_add(get_top_statement_block(ast), node);

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

    node->parent = top(ast->program.block_stack);
    ast_list_add(get_top_statement_block(ast), node);

    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_return(ASTNode* ast)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_RETURN,
        .next = NULL,
        .return_st = {
            .expression = NULL,
        },
    };

    node->parent = top(ast->program.block_stack);
    ast_list_add(get_top_statement_block(ast), node);

    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_while_do(ASTNode* ast, Symtable* symtable)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_WHILE_DO,
        .next = NULL,
        .while_do = {
            .condition = NULL,
            .statements = { },
            .symtable = symtable },
    };

    node->parent = top(ast->program.block_stack);
    ast_list_add(get_top_statement_block(ast), node);
    push(ast->program.block_stack, node);

    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_expression(ASTNode* ast, ASTNode* expression)
{
    expression->parent = top(ast->program.block_stack);
    ast_list_add(get_top_statement_block(ast), expression);

    ast->program.recent_node = expression;
    return expression;
}

// ===============================================

SymtableEntry* get_closest_symtable_entry(const ASTNode* node, const char* identifier)
{
    if (node == NULL)
        return NULL;

    const ASTNode* current = node->parent;

    while (current != NULL) {

        const Symtable* symtable = NULL;
        switch (current->type) {
            case AST_PROGRAM:
                symtable = current->program.global_symtable;
                break;
            case AST_FUNCTION:
                symtable = current->function.symtable;
                break;
            case AST_EXTENSION:
                symtable = current->extension.symtable;
                break;
            case AST_WHILE_DO:
                symtable = current->while_do.symtable;
                break;
            default:
                break;
        }

        if (symtable == NULL) {
            current = current->parent;
            continue;
        }

        const TNode* search_result = search(symtable, identifier);

        if (search_result == NULL) {
            current = current->parent;
            continue;
        }

        return search_result->value;
    }

    return NULL;
}

// ============================================================

ASTNode* ast_new_expression(const ASTExpression expression)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_EXPRESSION,
        .next = NULL,
        .parent = NULL,
        .expression = {
            .left = expression.left,
            .right = expression.right,
        }
    };

    return node;
}

ASTNode* ast_new_term(const char* identifier)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_TERM,
        .next = NULL,
        .parent = NULL,
        .term = {
            .identifier = identifier,
        },
    };

    return node;
}

ASTNode* ast_new_interpolated_string()
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_INTERPOLATED_STRING,
        .next = NULL,
        .parent = NULL,
        .interpolated_string = {
            .expressions = { },
        },
    };

    return node;
}

ASTNode* ast_new_literal(const ASTLiteral literal)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_LITERAL,
        .next = NULL,
        .parent = NULL,
        .literal = literal
    };

    return node;
}

// ========================================
