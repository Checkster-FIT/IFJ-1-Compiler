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
 * Checks:
 *  number and order of parameters
 *  data types of parameters
 *  expressions inside function
 *
 * @param fun_node Pointer to function AST node.
 * @return Data type of function return value.
 */
char fun_sem(ASTNode* fun_node);
int run_semantic(ASTNode* ast);

#endif // IFJ_1_COMPILER_SEMANTIC_H