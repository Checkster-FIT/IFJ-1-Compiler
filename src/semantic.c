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

// TODO
int handle_expression(ASTNode* node, char* result_type)
{
    int result = 0;
    char type;

    // TODO switch or if elses
    switch (node->)



    return 0;
}


int fun_sem(ASTNode* fun_node, char* fun_return_type)
{
    int result = 0;
    char result_type;

    ASTNode* child_node = fun_node->function.statements.first;

    while (child_node) {
        result = handle_expression(child_node, &result_type);
        child_node = child_node->next;
    }

    *fun_return_type = result_type;
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

     while (fun_node) {
         result = fun_sem(fun_node, &return_type);
         if (result != 0)
             return result;

         SymtableEntry* entry = get_closest_symtable_entry(fun_node, fun_node->function.identifier);

         if (!entry->data_type)
             entry->data_type = return_type;

         if (return_type != entry->data_type)
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