/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    semantic.h
 * @brief
 * @author  Wyslych Vojtech (xwyslyv00)
 */

#ifndef IFJ_1_COMPILER_SEMANTIC_H
#define IFJ_1_COMPILER_SEMANTIC_H

#include "ast.h"

char *type_full_name(char c);

ASTNode *find_function(ASTNode *node, const char *identifier);

int handle_block(ASTNodeList list, char *result_type);

int handle_statement(ASTNode *statement_node, char *result_type, char *dot_funcall_type);

int handle_expression(ASTNode *node, char *result_type);

int handle_function(ASTNode * node);

int main_check(ASTNode *fun_node, bool *main_found);

int run_semantic(ASTNode * ast);

#endif // IFJ_1_COMPILER_SEMANTIC_H