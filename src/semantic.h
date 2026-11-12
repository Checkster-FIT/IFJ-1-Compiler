/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    semantic.h
 * @brief
 * @author  Drahovska Andrea (xdrahoa00), Wyslych Vojtech (xwyslyv00)
 */

#ifndef IFJ_1_COMPILER_SEMANTIC_H
#define IFJ_1_COMPILER_SEMANTIC_H

#include "ast.h"

int handle_statement(ASTNode* statement_node, char* result_type);

int handle_expression(ASTNode* node, char* result_type);

int run_semantic(ASTNode* ast);

#endif // IFJ_1_COMPILER_SEMANTIC_H