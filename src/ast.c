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
        .line = 0,
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
            free_node_list(&node->term.arguments);
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
        case AST_IF:
            free_ast_nodes(node->if_statement.condition);
            free_ast_nodes(node->if_statement.then_node);
            free_ast_nodes(node->if_statement.else_node);
            break;
        case AST_THEN:
            free_node_list(&node->then_statement.statements);
            free_symtable(node->then_statement.symtable);
            break;
        case AST_ELSE:
            free_node_list(&node->else_statement.statements);
            free_symtable(node->else_statement.symtable);
            break;
        case AST_CONVERSION:
            free_ast_nodes(node->conversion.expression);
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
    if (node->type == AST_THEN)
        return &node->then_statement.statements;
    if (node->type == AST_ELSE)
        return &node->else_statement.statements;

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
        .line = 0,
        .extension = {
            .functions = {},
            .parameter_groups = {},
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
        .line = 0,
        .function = {
            .identifier = identifier,
            .statements = {},
            .parameter_groups = {},
            .symtable = symtable,
        },
    };

    if (target == AST_TARGET_FUNCTION) {
        node->parent = ast;
        ast_list_add(&node->parent->program.functions, node);
    } else if (target == AST_TARGET_EXTENSION) {
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
        .line = 0,
        .parameter_group = {
            .parameters = {},
        },
    };

    if (target == AST_TARGET_FUNCTION) {
        node->parent = ast->program.recent_function;
        ast_list_add(&node->parent->function.parameter_groups, node);
    } else if (target == AST_TARGET_EXTENSION) {
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
        .line = 0,
        .parameter = {
            .identifier = identifier,
        },
    };

    if (target == AST_TARGET_FUNCTION) {
        node->parent = ast->program.recent_function->function.parameter_groups.last;
        ast_list_add(&node->parent->parameter_group.parameters, node);
    } else if (target == AST_TARGET_EXTENSION) {
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
        .line = 0,
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
        .line = 0,
        .parent = NULL,
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
        .line = 0,
        .parent = NULL,
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
        .line = 0,
        .parent = NULL,
        .while_do = {
            .condition = NULL,
            .statements = {},
            .symtable = symtable },
    };

    node->parent = top(ast->program.block_stack);
    ast_list_add(get_top_statement_block(ast), node);
    push(ast->program.block_stack, node);

    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_if(ASTNode* ast)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    *node = (ASTNode) {
        .type = AST_IF,
        .next = NULL,
        .line = 0,
        .parent = NULL,
        .if_statement = {
            .condition = NULL,
            .then_node = NULL,
            .else_node = NULL,
        },
    };

    node->parent = top(ast->program.block_stack);
    ast_list_add(get_top_statement_block(ast), node);
    ast->program.recent_node = node;
    return node;
}

ASTNode* ast_add_if_branch(ASTNode* ast, ASTNodeType type, ASTNode* if_node, Symtable* symtable)
{
    ASTNode* node = malloc(sizeof(ASTNode));

    if (node == NULL)
        return NULL;

    if (type == AST_THEN) {
        *node = (ASTNode) {
            .type = AST_THEN,
            .next = NULL,
            .line = 0,
            .parent = NULL,
            .then_statement = {
                .symtable = symtable,
                .statements = {},
            },
        };
        if_node->if_statement.then_node = node;
    } else if (type == AST_ELSE) {
        *node = (ASTNode) {
            .type = AST_ELSE,
            .next = NULL,
            .line = 0,
            .parent = NULL,
            .else_statement = {
                .symtable = symtable,
                .statements = {},
            },
        };
        if_node->if_statement.else_node = node;
    }

    node->parent = if_node;
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

Symtable* get_node_symtable(const ASTNode* node)
{
    switch (node->type) {
        case AST_PROGRAM:
            return node->program.global_symtable;
        case AST_FUNCTION:
            return node->function.symtable;
        case AST_EXTENSION:
            return node->extension.symtable;
        case AST_WHILE_DO:
            return node->while_do.symtable;
        case AST_THEN:
            return node->then_statement.symtable;
        case AST_ELSE:
            return node->else_statement.symtable;
        default:
            return NULL;
    }
}

SymtableEntry* get_closest_defined_symtable_entry(const ASTNode* node, const char* identifier)
{
    if (node == NULL)
        return NULL;

    const ASTNode* current = node->parent;

    while (current != NULL) {
        const Symtable* symtable = get_node_symtable(current);

        if (symtable == NULL) {
            current = current->parent;
            continue;
        }

        const TNode* search_result = search(symtable, identifier);

        if (search_result == NULL || search_result->value == NULL) {
            current = current->parent;
            continue;
        }

        // Skip undefined entries
        if (((SymtableEntry*)search_result->value)->defined == false) {
            current = current->parent;
            continue;
        }

        return search_result->value;
    }

    return NULL;
}

SymtableEntry* get_closest_symtable_entry(const ASTNode* node, const char* identifier)
{
    if (node == NULL)
        return NULL;

    const ASTNode* current = node->parent;

    while (current != NULL) {
        const Symtable* symtable = get_node_symtable(current);

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
        .line = 0,
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
        .line = 0,
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
        .line = 0,
        .interpolated_string = {
            .expressions = {},
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
        .line = 0,
        .literal = literal
    };

    return node;
}

// ========================================

ASTNode* ast_insert_conversion(ASTNode* node, const char new_type)
{
    ASTNode* new_node = malloc(sizeof(ASTNode));

    if (new_node == NULL)
        return NULL;

    *new_node = *node;

    *node = (ASTNode) {
        .type = AST_CONVERSION,
        .next = NULL,
        .parent = new_node->parent,
        .line = node->line,
        .conversion = {
            .result_type = new_type,
            .expression = NULL,
        },
    };

    node->conversion.expression = new_node;

    return node;
}
