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

char* type_full_name(char c)
{
    switch (c) {
        case 'i':
            return "Int";
        case 'd':
            return "Double";
        case 's':
            return "String";
        case 'u':
            return "Unit";
        case 'b':
            return "Boolean";
        case 'a':
            return "Any";
        default:
            return "unknown";
    }
}

int handle_block(ASTNodeList list, char* result_type)
{
    int result = 0;
    *result_type = 'u';

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
        case AST_IF: {
            char then_t = 'u';
            char else_t = 'u';
            result = handle_expression(statement_node->if_statement.condition, result_type);
            if (result)
                return result;

            if (*result_type != 'b')
                return print_error(SEMANTIC_IF_WHILE_CONDITION_NOT_BOOL, statement_node->line, "if");

            ASTNode* then_n = statement_node->if_statement.then_node;
            ASTNode* else_n = statement_node->if_statement.else_node;

            result = handle_block(then_n->then_statement.statements, result_type);
            if (result)
                return result;

            then_t = *result_type;

            if (else_n) {
                result = handle_block(else_n->else_statement.statements, result_type);
                if (result)
                    return result;

                else_t = *result_type;
            } else
                else_t = 'u';

            if (then_t == else_t && then_t == 'r')
                *result_type = 'r';

            else if (then_t == else_t && then_t == 'u')
                *result_type = 'u';

            else
                *result_type = 'e';

            return 0;
        }

        case AST_WHILE_DO:
            result = handle_expression(statement_node->while_do.condition, result_type);
            if (result)
                return result;

            if (*result_type != 'b')
                return print_error(SEMANTIC_IF_WHILE_CONDITION_NOT_BOOL, statement_node->line, "while");

            result = handle_block(statement_node->while_do.statements, result_type);
            if (result)
                return result;

            *result_type = 'u';
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
            if (!entry)
                return print_error(SEMANTIC_USE_OF_UNDEFINED_VAL_FUN, statement_node->line, statement_node->vardef.identifier);
            result = handle_expression(statement_node->vardef.expression, result_type);
            if (result)
                return result;
            if (!entry->data_type)
                entry->data_type = *result_type;

            // e.x. `val x: Double = 3`
            if (entry->data_type == 'd' && *result_type == 'i') {
                ASTNode* new = ast_insert_conversion(statement_node->vardef.expression, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, statement_node->line, "conversion node");
                *result_type = 'd';
            }

            printf("%c", entry->data_type);
            if (entry->data_type != *result_type)
                return print_error(SEMANTIC_NOT_MATCHING_VAR_TYPE, statement_node->line, type_full_name(*result_type), type_full_name(entry->data_type));

            if (entry->defined)
                return print_error(SEMANTIC_VARIABLE_OR_FUNCTION_REDEFINITION, statement_node->line);

            entry->defined = true;
            *result_type = 'u';
            return 0;

        case AST_VAR_ASSIGN:
            entry = get_closest_symtable_entry(statement_node, statement_node->assignment.target);
            if (!entry)
                return print_error(SEMANTIC_USE_OF_UNDEFINED_VAL_FUN, statement_node->line, statement_node->assignment.target);
            if (entry->function || entry->constant)
                return print_error(SEMANTIC_WRITE_INTO_CONST_OR_FUN, statement_node->line, entry->identifier);
            if (!entry->defined)
                return print_error(SEMANTIC_USE_OF_UNDEFINED_VAL_FUN, statement_node->line, entry->identifier);
            result = handle_expression(statement_node->assignment.expression, result_type);
            if (result)
                return result;

            if (entry->data_type == 'd' && *result_type == 'i') {
                ASTNode* new = ast_insert_conversion(statement_node->assignment.expression, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, statement_node->line, "conversion node");
                *result_type = 'd';
            }

            if (entry->data_type != *result_type) {
                return print_error(SEMANTIC_NOT_MATCHING_VAR_TYPE, statement_node->line, type_full_name(entry->data_type), type_full_name(*result_type));
            }

            *result_type = 'u';
            return 0;

        case AST_RETURN: {
            ASTNode* node = statement_node->parent;
            while (node->type != AST_FUNCTION)
                node = node->parent;

            entry = get_closest_symtable_entry(node, node->function.identifier);
            if (!entry)
                return print_error(SEMANTIC_STANDALONE_RETURN, statement_node->line);

            ASTNode* expr = statement_node->return_st.expression;

            if (!expr) {
                if (entry->data_type != 'u')
                    return print_error(SEMANTIC_RETURN_NOT_ALLOWED, statement_node->line, entry->identifier, type_full_name(entry->data_type));

                *result_type = 'r';
                return 0;
            }

            result = handle_expression(statement_node->return_st.expression, result_type);
            if (result)
                return result;

            if (entry->data_type == 'u') {
                *result_type = 'r';
                return 0;
            }

            // e.x. `return 5` in f(): Double
            if (entry->data_type == 'd' && *result_type == 'i') {
                ASTNode* new = ast_insert_conversion(statement_node->return_st.expression, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, statement_node->line, "conversion node");
                *result_type = 'r';
                return 0;
            }

            if (!entry->data_type)
                return print_error(SEMANTIC_RETURN_IN_F_WITHOUT_RETURN_TYPE, statement_node->line, entry->identifier);

            if (entry->data_type != *result_type)
                return print_error(SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE, statement_node->line);

            *result_type = 'r';
            return 0;
        }

        case AST_LITERAL: {
            char t = statement_node->literal.type;
            switch (t) {
                case AST_INTEGER:
                    *result_type = 'i';
                    break;
                case AST_DOUBLE:
                    *result_type = 'd';
                    break;
                case AST_BOOLEAN:
                    *result_type = 'b';
                    break;
                case AST_STRING:
                    *result_type = 's';
                    break;
                case AST_UNIT:
                    *result_type = 'u';
                    break;
                default:
                    *result_type = 'u';
                    break;
            }

            return 0;
        }

        case AST_EXPRESSION:
            return handle_expression(statement_node, result_type);

        case AST_TERM:
            // get closest either function or defined variable
            entry = get_closest_defined_symtable_entry(statement_node, statement_node->term.identifier);
            if (!entry)
                return print_error(SEMANTIC_USE_OF_UNDEFINED_VAL_FUN, statement_node->line, statement_node->term.identifier);
            if (entry->function) {
                char* param_string = entry->parameters->chars;
                int i = 0;

                for (ASTNode* group = statement_node->term.argument_groups.first; group; group = group->next) {
                    if (param_string[i] != '#')
                        return print_error(SEMANTIC_FUNCALL_PARAMETER_MISMATCH, statement_node->line, entry->identifier);
                    i++;

                    for (ASTNode* arg = group->argument_group.arguments.first; arg; arg = arg->next) {
                        if (param_string[i] == '#' || param_string[i] == '\0')
                            return print_error(SEMANTIC_FUNCALL_PARAMETER_MISMATCH, statement_node->line, entry->identifier);

                        result = handle_expression(arg, result_type);
                        if (result)
                            return result;

                        if (*result_type == 'i' && param_string[i] == 'd') {
                            ASTNode* new = ast_insert_conversion(arg, 'd');
                            if (!new)
                                return print_error(MEMORY_ALLOCATION_ERROR, statement_node->line, "conversion node");
                            *result_type = 'd';
                        }

                        // check if the parameter is not `Any` type
                        if (*result_type != param_string[i] && param_string[i] != 'a')
                            return print_error(SEMANTIC_FUNCALL_PARAMETER_TYPE_MISMATCH, statement_node->line);

                        i++;
                    }
                }
                if (param_string[i] != '\0')
                    return print_error(SEMANTIC_FUNCALL_PARAMETER_MISMATCH, statement_node->line, entry->identifier);

                *result_type = entry->data_type;
                statement_node->type = AST_FUNCALL;
                return 0;
            }

            ASTNode* par = statement_node->term.argument_groups.first;
            if (par)
                return print_error(SEMANTIC_ARGUMENT_PASSED_TO_VARIABLE, statement_node->line, entry->identifier);

            *result_type = entry->data_type;
            statement_node->type = AST_VAR_USE;
            return 0;

        case AST_INTERPOLATED_STRING:
            result = handle_block(statement_node->interpolated_string.expressions, result_type);
            if (result)
                return result;

            *result_type = 's';
            return 0;

        case AST_FUNCALL:
        case AST_VAR_USE:
            entry = get_closest_defined_symtable_entry(statement_node, statement_node->term.identifier);
            if (!entry)
                return print_error(SEMANTIC_USE_OF_UNDEFINED_VAL_FUN, statement_node->line, statement_node->term.identifier);

            *result_type = entry->data_type;
            return 0;

        case AST_CONVERSION:
            *result_type = statement_node->conversion.result_type;
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
    char* op = "";

    if (node->type != AST_EXPRESSION)
        return handle_statement(node, result_type);

    int oper = node->expression.operation;

    switch (oper) {
        case OP_NOT:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;

            if (*result_type != 'b')
                return print_error(SEMANTIC_NEGATION_OF_NON_BOOL_TYPE, node->line, type_full_name(*result_type));

            return 0;

        case OP_UNARY_PLUS:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;

            if (*result_type != 'i' && *result_type != 'd')
                return print_error(SEMANTIC_UNARY_OPERATION_NOT_OVER_NUMBER, node->line, "+", type_full_name(*result_type));

            return 0;

        case OP_UNARY_MINUS:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;

            if (*result_type != 'i' && *result_type != 'd')
                return print_error(SEMANTIC_UNARY_OPERATION_NOT_OVER_NUMBER, node->line, "-", type_full_name(*result_type));

            return 0;

        case OP_MUL:
        case OP_DIV:
        case OP_ADD:
        case OP_SUB:
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
                ASTNode* new = ast_insert_conversion(node->expression.left, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'd';
                return 0;
            }

            if (*result_type == 'd' && right_result_type == 'i') {
                ASTNode* new = ast_insert_conversion(node->expression.right, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'd';
                return 0;
            }

            if (oper == OP_ADD && *result_type == 's' && right_result_type == 's')
                return 0;

            op = "";
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
                case OP_ADD:
                    op = "+";
                    return print_error(SEMANTIC_NOT_OVER_NUMBERS_OR_STRINGS, node->line, op, type_full_name(*result_type), type_full_name(right_result_type));
                default:
                    break;
            }

            return print_error(SEMANTIC_NUMERIC_OPERATION_NOT_OVER_NUMBERS, node->line, op, type_full_name(*result_type), type_full_name(right_result_type));

        case OP_LESS:
        case OP_LESS_EQUAL:
        case OP_GREATER:
        case OP_GREATER_EQUAL:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;
            result = handle_expression(node->expression.right, &right_result_type);
            if (result)
                return result;

            if (*result_type == 'd' && right_result_type == 'd') {
                *result_type = 'b';
                return 0;
            }

            if (*result_type == 'i' && right_result_type == 'i') {
                *result_type = 'b';
                return 0;
            }

            if (*result_type == 'i' && right_result_type == 'd') {
                ASTNode* new = ast_insert_conversion(node->expression.left, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'b';
                return 0;
            }

            if (*result_type == 'd' && right_result_type == 'i') {
                ASTNode* new = ast_insert_conversion(node->expression.right, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'b';
                return 0;
            }

            if (*result_type == 's' && right_result_type == 's') {
                *result_type = 'b';
                return 0;
            }

            op = "";
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

            return print_error(SEMANTIC_NOT_OVER_NUMBERS_OR_STRINGS, node->line, op, type_full_name(*result_type), type_full_name(right_result_type));

        case OP_EQUAL:
        case OP_NOT_EQUAL:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;
            result = handle_expression(node->expression.right, &right_result_type);
            if (result)
                return result;

            if (*result_type == right_result_type) {
                *result_type = 'b';
                return 0;
            }

            if (*result_type == 'i' && right_result_type == 'd') {
                ASTNode* new = ast_insert_conversion(node->expression.left, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'b';
                return 0;
            }

            if (*result_type == 'd' && right_result_type == 'i') {
                ASTNode* new = ast_insert_conversion(node->expression.right, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'b';
                return 0;
            }

            op = "";
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

            return print_error(SEMANTIC_EQUAL_NOT_EQUAL_TYPE_MISMATCH, node->line, op, type_full_name(*result_type), type_full_name(right_result_type));

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
                    return print_error(SEMANTIC_BINARY_OP_NOT_BOOL, node->line, "&&", type_full_name(*result_type), type_full_name(right_result_type));

                return print_error(SEMANTIC_BINARY_OP_NOT_BOOL, node->line, "||", type_full_name(*result_type), type_full_name(right_result_type));
            }

            return 0;

        default:
            break;
    }

    return result;
}

bool is_main(ASTNode* fun_node)
{
    SymtableEntry* entry = get_closest_symtable_entry(fun_node, fun_node->function.identifier);
    return entry->main;
}

/** TODO:
 * extension handling
 * errors should receive full data type name, not just i, s, d,...
 * return type of function called before its definition
 * `foo()` handling
 */
int run_semantic(ASTNode* ast)
{

    int result = 0;
    char return_type = 'u';

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

        // when else and if types don't match, 'e' is propagated, but it's valid for Unit function
        if (return_type == 'e' && entry->data_type != 'u')
            return print_error(SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE, fun_node->line);

        if (!entry->data_type) {
            entry->data_type = return_type;
        }

        // e.x. `def f(): Double = \n 3+3`
        if (return_type == 'i' && entry->data_type == 'd') {
            ASTNode* new = ast_insert_conversion(fun_node->function.statements.last, 'd');
            if (!new)
                return print_error(MEMORY_ALLOCATION_ERROR, fun_node->line, "conversion node");
            return_type = 'd';
        }

        if (return_type != entry->data_type && return_type != 'r' && entry->data_type != 'u')
            return print_error(SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE, fun_node->line);

        if (is_main(fun_node)) {
            if (main_found)
                return print_error(SEMANTIC_MULTIPLE_MAIN, fun_node->line);

            if (entry->parameters->len != 1) // `#` = no args
                return print_error(SEMANTIC_MAIN_WITH_PARAMETERS, fun_node->line);

            if (entry->data_type != 'u')
                return print_error(SEMANTIC_MAIN_NOT_UNIT, fun_node->line);

            main_found = true;
        }

        fun_node = fun_node->next;
    }

    if (!main_found)
        return print_error(SEMANTIC_NO_MAIN, 0);

    return 0;
}