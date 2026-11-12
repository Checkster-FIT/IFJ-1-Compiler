/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    semantic.h
 * @brief
 * @author  Drahovska Andrea (xdrahoa00), Wyslych Vojtech (xwyslyv00)
 */

#ifndef IFJ_1_COMPILER_SEMANTIC_H
#define IFJ_1_COMPILER_SEMANTIC_H

#include "ast.h"

/**
 * @brief Performs semantic check on function node.
 *
 * @param fun_node Pointer to function AST node.
 * @param fun_return_type Pointer into which the return value of inspected function is stored.
 * @return 0 on success, else on error
 */
int fun_sem(ASTNode* fun_node, char* fun_return_type);
int run_semantic(ASTNode* ast);

#endif // IFJ_1_COMPILER_SEMANTIC_H