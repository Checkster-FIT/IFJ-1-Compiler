/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    semantic.c
 * @brief   Implementation of semantic analysis.
 * @author  Drahovska Andrea (xdrahoa00), Wyslych Vojtech (xwyslyv00)
 */

#include "semantic.h"
#include "ast.h"
#include "debug.h"
#include "error.h"

int handle_block(ASTNodeList list, char* result_type)
{
    int result = 0;

    ASTNode* statement = list.first;

    while (statement) {
        result = handle_statement(statement, result_type);
        if (result != 0)
            break;
        statement = statement->next;
    }

    return result;
}

int handle_statement(ASTNode* statement_node, char* result_type)
{
    int result = 0;
    SymtableEntry* entry;

    switch (statement_node->type) {
        case AST_IF:
            result = handle_expression(statement_node->if_statement.condition, result_type);
            if (result)
                return result;

            if (*result_type != 'b')
                return print_error(IF_WHILE_CONDITION_NOT_BOOL, 0, "if");

            ASTNode* then_n = statement_node->if_statement.then_node;
            ASTNode* else_n = statement_node->if_statement.else_node;

            result = handle_block(then_n->then_statement.statements, result_type);
            if (result)
                return result;

            if (else_n) {
                result = handle_block(else_n->else_statement.statements, result_type);
                if (result)
                    return result;
            }

            return 0;

        case AST_WHILE_DO:
            result = handle_expression(statement_node->while_do.condition, result_type);
            if (result)
                return result;

            if (*result_type != 'b')
                return print_error(IF_WHILE_CONDITION_NOT_BOOL, 0, "while");

            result = handle_block(statement_node->while_do.statements, result_type);
            if (result)
                return result;

            return 0;

        case AST_THEN:
            result = handle_block(statement_node->then_statement.statements, result_type);
            if (result)
                return result;

            return 0;

        case AST_ELSE:
            result = handle_block(statement_node->else_statement.statements, result_type);
            if (result)
                return result;

            return 0;

        case AST_VAR_DEF:
            entry = get_closest_symtable_entry(statement_node, statement_node->vardef.identifier);
            result = handle_expression(statement_node->vardef.expression, result_type);
            if (result)
                return result;
            if (!entry->data_type)
                entry->data_type = *result_type;
            else if (entry->data_type != *result_type) {
                return print_error(SEMANTIC_VARIABLE_OR_FUNCTION_REDEFINITION, 0);
            }

            return 0;

        case AST_VAR_ASSIGN:
            entry = get_closest_symtable_entry(statement_node, statement_node->variable.identifier);
            result = handle_expression(statement_node->assignment.expression, result_type);
            if (result)
                return result;
            if (!entry->data_type)
                entry->data_type = *result_type;
            else if (entry->data_type != *result_type) {
                return print_error(SEMANTIC_VARIABLE_OR_FUNCTION_REDEFINITION, 0);
            }

            return 0;

        case AST_RETURN:
            result = handle_expression(statement_node->return_st.expression, result_type);
            if (result)
                return result;

            ASTNode* node = statement_node->parent;

            while (node->type != AST_FUNCTION)
                node = node->parent;

            entry = get_closest_symtable_entry(node, node->function.identifier);

            if (entry->data_type == 'u') {
                *result_type = 'u';
                return 0;
            }

            if (*result_type != entry->data_type)
                return print_error(SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE, 0);

            return 0;

        case AST_LITERAL:
        case AST_EXPRESSION:
        case AST_TERM:
            return handle_expression(statement_node, result_type);

        case AST_INTERPOLATED_STRING:
            result = handle_block(statement_node->interpolated_string.expressions, result_type);
            if (result)
                return result;

            *result_type = 's';
            return 0;

        default:
            break;
    }

    return result;
}

int handle_expression(ASTNode* node, char* result_type)
{
    int result = 0;
    char right_result_type;
    int oper = node->expression.operation;

    switch (oper) {
        case OP_NOT:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;

            if (*result_type != 'b')
                return print_error(SEMANTIC_NEGATION_OF_NON_BOOL_TYPE, 0, *result_type);

            return 0;

        case OP_UNARY_PLUS:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;

            if (*result_type != 'i' && *result_type != 'f')
                return print_error(SEMANTIC_UNARY_OPERATION_NOT_OVER_NUMBER, 0, "+", *result_type);

            return 0;

        case OP_UNARY_MINUS:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;

            if (*result_type != 'i' && *result_type != 'f')
                return print_error(SEMANTIC_UNARY_OPERATION_NOT_OVER_NUMBER, 0, "-", *result_type);

            return 0;

        case OP_MUL:
        case OP_DIV:
        case OP_ADD:
        case OP_SUB:
        case OP_LESS:
        case OP_LESS_EQUAL:
        case OP_GREATER:
        case OP_GREATER_EQUAL:
        case OP_EQUAL:
        case OP_NOT_EQUAL:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;
            result = handle_expression(node->expression.right, &right_result_type);
            if (result)
                return result;

            if (*result_type == 'd' && right_result_type == 'd')
                return 0;
            if (*result_type == 'i' && right_result_type == 'i')
                return 0;

            if (*result_type == 'i' && right_result_type == 'd') {
                // TODO: vytvorit pozemek
                *result_type = 'd';
                return 0;
            }

            if (*result_type == 'd' && right_result_type == 'i') {
                // TODO: vytvorit pozemek
                *result_type = 'd';
                return 0;
            }

            // <, >, ==, ... *result_type = 'b'
            if (oper != OP_MUL && oper != OP_DIV && oper != OP_SUB) {
                if (*result_type == 's' && right_result_type == 's') {
                    if (oper != OP_ADD)
                        *result_type = 'b';

                    return 0;
                }

                if ((oper == OP_EQUAL || oper == OP_NOT_EQUAL) && *result_type == right_result_type) {
                    *result_type = 'b';
                    return 0;
                }
            }

            char* op = "";
            switch (oper) {
                case OP_MUL:
                    op = "*";
                    break;
                case OP_DIV:
                    op = "/";
                    break;
                case OP_SUB:
                    op = "-";
                    break;
                default:
                    break;
            }

            if (oper == OP_MUL || oper == OP_DIV || oper == OP_SUB)
                return print_error(SEMANTIC_NUMERIC_OPERATION_NOT_OVER_NUMBERS, 0, op, *result_type, right_result_type);

            switch (oper) {
                case OP_LESS:
                    op = "<";
                    break;
                case OP_LESS_EQUAL:
                    op = "<=";
                    break;
                case OP_GREATER:
                    op = ">";
                    break;
                case OP_GREATER_EQUAL:
                    op = ">=";
                    break;
                default:
                    break;
            }

            if (oper != OP_EQUAL && oper != OP_NOT_EQUAL)
                return print_error(SEMANTIC_NOT_OVER_NUMBERS_OR_STRINGS, 0, op, *result_type, right_result_type);

            switch (oper) {
                case OP_EQUAL:
                    op = "==";
                    break;
                case OP_NOT_EQUAL:
                    op = "!=";
                    break;
                default:
                    break;
            }

            return print_error(SEMANTIC_EQUAL_NOT_EQUAL_TYPE_MISMATCH, 0, op, *result_type, right_result_type);

        case OP_AND:
        case OP_OR:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;
            result = handle_expression(node->expression.right, &right_result_type);
            if (result)
                return result;

            if (*result_type != 'b' || right_result_type != 'b') {
                if (oper == OP_AND)
                    return print_error(SEMANTIC_BINARY_OP_NOT_BOOL, 0, "&&", *result_type, right_result_type);

                return print_error(SEMANTIC_BINARY_OP_NOT_BOOL, 0, "||", *result_type, right_result_type);
            }

            return 0;

        default:
            return result;
    }

    return result;
}

bool is_main(ASTNode* fun_node)
{
    SymtableEntry* entry = get_closest_symtable_entry(fun_node, fun_node->function.identifier);
    if (entry->main)
        return true;

    return false;
}

/** TODO:
 * GO ON WITH THIS: change ASTTerm to function or expression
 * extension handling
 * errors should receive full data type name, not just i, s, d,...
 */
int run_semantic(ASTNode* ast)
{

    int result = 0;
    char return_type = 'e';

    if (ast->type != AST_PROGRAM) {
        return print_error(CORRUPTED_MEMORY, 0);
    }

    ASTNode* fun_node = ast->program.functions.first;

    bool main_found = false;

    // go through functions
    while (fun_node) {
        result = handle_block(fun_node->function.statements, &return_type);
        if (result)
            return result;

        SymtableEntry* entry = get_closest_symtable_entry(fun_node, fun_node->function.identifier);

        if (!entry->data_type)
            entry->data_type = return_type;

        // ignore found return type when declared as Unit
        if (return_type != entry->data_type && entry->data_type != 'u')
            return print_error(SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE, 0);

        if (is_main(fun_node)) {
            if (main_found)
                return print_error(SEMANTIC_MULTIPLE_MAIN, 0);

            main_found = true;
        }

        fun_node = fun_node->next;
    }

    if (!main_found)
        return print_error(SEMANTIC_NO_MAIN, 0);

    return 0;
}