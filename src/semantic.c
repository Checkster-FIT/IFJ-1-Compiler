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

/** TODO:
 *
 *  parameters count
 *
 *  parameters data types
 *
 *  expressions
 *
 */
char fun_sem(ASTNode* fun_node)
{




    return e;
}

bool is_main(ASTNode* fun_node)
{
    SymtableEntry* entry = get_scoped_symtable_entry(fun_node, fun_node->function.identifier);
    if (entry->main)
        return true;

    return false;
}

/** TODO:
 * change ASTTerm to function or expression
 *
 */
int run_semantic(ASTNode* ast)
{

    if (ast->type != AST_PROGRAM) {
        return print_error(CORRUPTED_MEMORY, 0);
    }

    ASTNode* fun_node = ast->program.functions.first;

    bool main_found = false;

    while (fun_node) {
        char ret = fun_sem(fun_node);
        SymtableEntry* entry = get_scoped_symtable_entry(fun_node, fun_node->function.identifier);
        if (!entry->data_type)
            entry->data_type = ret;

        if (ret != entry->data_type)
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