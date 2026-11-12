/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator.c
 * @brief
 * @author
 */

#include "generator.h"
#include "ast.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

int run_generator(ASTNode* ast)
{
    print_ast(ast);
    return 0;
}

void generate_program(ASTNode* node)
{
    printf(".IFJcode26\n\n");
    if (node == NULL || node->type != AST_PROGRAM) {
        fprintf(stderr, "generate_program: expected AST_PROGRAM\n");
        exit(99);
    }

    SymtableEntry* main_entry = symtable_main_search(node->program.global_symtable);
    if (main_entry == NULL) {
        fprintf(stderr, "generate_program: entry function not found\n");
        exit(99);
    }

    printf("JUMP %s\n\n", main_entry->identifier);

    ASTNode* helper_node = node->program.functions.first;
    while (helper_node != NULL) {
        generate_function(helper_node);
        helper_node = helper_node->next;
    }
    printf("EXIT int@0\n");
}

void generate_function(ASTNode* node)
{
    printf("LABEL %s\n", node->function.identifier);

    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");

    generate_local_variables(node->function.symtable);
    generate_parameters(&node->function.parameter_groups);
    generate_statements(node->function.statements.first);

    printf("POPFRAME\n");
    printf("RETURN\n\n");
}

void generate_local_variables_recursive(TNode* node)
{
    if (node == NULL) {
        return;
    }
    generate_local_variables_recursive(node->left);
    SymtableEntry* entry = (SymtableEntry*)node->value;
    printf("DEFVAR LF@%s\n", entry->identifier);
    generate_local_variables_recursive(node->right);
}

void generate_local_variables(Symtable* symtable)
{
    if (symtable == NULL) {
        return;
    }
    if (symtable->root == NULL) {
        return;
    }
    printf("DEFVAR LF@$discard\n");
    generate_local_variables_recursive(symtable->root);
}

void generate_parameters(ASTNodeList* param_group)
{
    if (param_group == NULL) {
        return;
    }
    ASTNode* param = param_group->first;
    while (param != NULL) {
        if (param->parameter.identifier != NULL) {
            printf("POPS LF@%s\n", param->parameter.identifier);
        }
        param = param->next;
    }
}

void generate_expression(ASTNode* node)
{
    if (node == NULL) {
        return;
    }
    switch (node->type) {
        case AST_LITERAL:
            char* type;
            switch (node->literal.type) {
                case AST_INTEGER:
                    type = "int";
                    printf("PUSHS %s@%d\n", type, node->literal.val_integer);
                    break;
                case AST_DOUBLE:
                    type = "float";
                    printf("PUSHS %s@%a\n", type, node->literal.val_double);
                    break;
                case AST_BOOLEAN:
                    type = "bool";
                    printf("PUSHS %s@%hhd\n", type, node->literal.val_bool);
                    break;
                case AST_STRING:
                    type = "string";
                    printf("PUSHS %s@%s\n", type, node->literal.val_string);
                    break;
                case AST_UNIT:
                    printf("PUSHS nil@nil\n");
                    break;
                case AST_UNKNOWN_LITERAL:
                    fprintf(stderr, "generate_expression: unknown literal\n");
                    exit(99);
            }
            break;
        case AST_TERM:
            printf("PUSHS LF@%s\n", node->term.identifier);
            break;
        case AST_EXPRESSION:
            switch (node->expression.operation) {
                case OP_MUL:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("MULS\n");
                    break;
                case OP_DIV:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("DIVS\n");
                    break;
                case OP_ADD:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("ADDS\n");
                    break;
                case OP_SUB:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("SUBS\n");
                    break;
                case OP_AND:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("ANDS\n");
                    break;
                case OP_OR:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("ORS\n");
                    break;
                case OP_NOT:
                    generate_expression(node->expression.left);
                    printf("NOTS\n");
                    break;
                case OP_UNARY_PLUS:
                    break;
                case OP_UNARY_MINUS:
                    printf("PUSHS int@0\n");
                    generate_expression(node->expression.left);
                    printf("SUBS\n");
                    break;
                case OP_LESS:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("LTS\n");
                    break;
                case OP_GREATER:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("GTS\n");
                    break;
                case OP_EQUAL:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("EQS\n");
                    break;
                case OP_LESS_EQUAL:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("GTS\n");
                    printf("NOTS\n");
                    break;
                case OP_GREATER_EQUAL:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("LTS\n");
                    printf("NOTS\n");
                    break;
                case OP_NOT_EQUAL:
                    generate_expression(node->expression.left);
                    generate_expression(node->expression.right);
                    printf("EQS\n");
                    printf("NOTS\n");
                    break;
                case OP_NONE:
                    fprintf(stderr, "generate_expression: operator isn't set\n");
                    exit(99);
                case OP_TERMINAL:
                    fprintf(stderr, "generate_expression: operator type is terminal\n");
                    exit(99);
                default:
                    fprintf(stderr, "generate_expression: accessed wrong operand type\n");
                    exit(99);
            }
            break;
        case AST_INTERPOLATED_STRING:
            ASTNode* expr = node->interpolated_string.expressions.first;
            while (expr != NULL) {
                generate_expression(expr);
                expr = expr->next;
            }
            break;
        case AST_FUNCTION:
            break;
        default:
            fprintf(stderr, "generate_expression: accessed wrong node type\n");
            exit(99);
    }
}

void generate_statements_switch(ASTNode* statement, bool is_last)
{
    switch (statement->type) {
        case AST_VAR_ASSIGN:
            generate_expression(statement->assignment.expression);
            printf("POPS LF@%s\n", statement->assignment.target);
            break;
        case AST_VAR_DEF:
            generate_expression(statement->vardef.expression);
            printf("POPS LF@%s\n", statement->vardef.identifier);
            break;
        case AST_WHILE_DO:
            static int while_counter = 0;
            while_counter += 1;
            printf("LABEL while_start_%d\n", while_counter);
            printf("PUSHS bool@false\n");
            generate_expression(statement->while_do.condition);
            printf("JUMPIFEQS while_end_%d\n", while_counter);
            generate_statements(statement->while_do.statements.first);
            printf("JUMP while_start_%d\n", while_counter);
            printf("LABEL while_end_%d\n", while_counter);
            break;
        case AST_RETURN:
            generate_expression(statement->return_st.expression);
            printf("POPFRAME\n");
            printf("RETURN\n");
            break;
        case AST_FUNCTION:
            printf("CREATEFRAME\n");
            ASTNode* arg = statement->function.parameter_groups.first;
            while (arg != NULL) {
                generate_expression(arg);
                arg = arg->next;
                printf("CALL %s\n", statement->function.identifier);
            }
            printf("CALL %s\n", statement->function.identifier);
            break;
        case AST_TERM:
        case AST_EXPRESSION:
            generate_expression(statement);
            if (!is_last) {
                printf("POPS LF@$discard\n");
            }
            break;
        case AST_IF:
            static int if_counter = 0;
            if_counter += 1;
            int current_if = if_counter;
            printf("PUSHS bool@false\n");
            generate_expression(statement->if_statement.condition);
            printf("JUMPIFEQS if_else_%d\n", current_if);
            generate_statements(statement->if_statement.then_node);
            printf("JUMP if_end_%d\n", current_if);
            printf("LABEL if_else_%d\n", current_if);
            if (statement->if_statement.else_node != NULL)
                generate_statements(statement->if_statement.else_node);
            printf("LABEL if_end_%d\n", current_if);
            break;
        case AST_ELSE:
            generate_statements(statement->if_statement.else_node);
            break;
        case AST_THEN:
            generate_statements(statement->if_statement.then_node);
            break;
        default:
            fprintf(stderr, "generate_statements_switch: accessed wrong statement type %d\n", statement->type);
            exit(99);
    }
}

void generate_statements(ASTNode* statement)
{
    bool is_last = false;
    while (statement != NULL) {
        if (statement->next == NULL) {
            is_last = true;
        }
        generate_statements_switch(statement, is_last);
        statement = statement->next;
    }
}
