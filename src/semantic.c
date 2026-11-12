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
            return handle_expression(statement_node->if_statement.condition, result_type);

        case AST_WHILE_DO:
            result = handle_expression(statement_node->while_do.condition, result_type);
            if (result) return result;

            result = handle_block(statement_node->while_do.statements, result_type);
            if (result) return result;

            *result_type = 'u';
            return 0;

        case AST_THEN:
            result = handle_block(statement_node->then_statement.statements, result_type);
            if (result) return result;

            *result_type = 'u';
            return 0;

        case AST_ELSE:
            result = handle_block(statement_node->else_statement.statements, result_type);
            if (result) return result;

            *result_type = 'u';
            return 0;

        // -------------------- TODO: ASSIGNMENT VS DEFINITION VS BOTH

        case AST_VAR_DEF:
            entry = get_closest_symtable_entry(statement_node, statement_node->vardef.identifier);
            result = handle_expression(statement_node->vardef.expression, result_type);
            if (result) return result;
            if (!entry->data_type)
                entry->data_type = *result_type;
            else if (entry->data_type != *result_type) {
                return print_error(SEMANTIC_VARIABLE_OR_FUNCTION_REDEFINITION, 0);
            }

            return 0;

        case AST_VAR_ASSIGN:
            entry = get_closest_symtable_entry(statement_node, statement_node->variable.identifier);
            result = handle_expression(statement_node->variable.expression, result_type);
            if (result) return result;
            if (!entry->data_type)
                entry->data_type = *result_type;
            else if (entry->data_type != *result_type) {
                return print_error(SEMANTIC_VARIABLE_OR_FUNCTION_REDEFINITION, 0);
            }

            return 0;

        // --------------------

        case AST_RETURN:
            result = handle_expression(statement_node->return_st.expression, result_type);
            if (result) return result;
            // TODO: if (result_type != get_closest_symtable_entry(statement_node.parent.i)->data_type)
                return print_error(SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE, 0);
            break;

        case AST_EXPRESSION:
            return handle_expression(statement_node, result_type);

        case AST_LITERAL:

        case AST_TERM:
        case AST_INTERPOLATED_STRING:
            result = handle_expression(statement_node, result_type);
            return result;

        default:
            break;
    }

    return result;
}

int handle_expression(ASTNode* node, char* result_type)
{
    int result = 0;

    switch (node->expression.operation) {
        case
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
 * change ASTTerm to function or expression
 * extension handling
 *
 */
int run_semantic(ASTNode* ast)
{

    int result = 0;
    char return_type;

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