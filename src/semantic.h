/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    semantic.h
 * @brief
 * @author  Wyslych Vojtech (xwyslyv00)
 */

#ifndef IFJ_1_COMPILER_SEMANTIC_H
#define IFJ_1_COMPILER_SEMANTIC_H

#include "ast.h"

char* type_full_name(char c);

int handle_block(ASTNodeList list, char* result_type);

int handle_statement(ASTNode* statement_node, char* result_type, char dot_funcall_type);

int handle_expression(ASTNode* node, char* result_type);

int handle_function(ASTNode* node, char* result_type, bool* main_found);

bool is_main(ASTNode* fun_node);

int run_semantic(ASTNode* ast);

#endif // IFJ_1_COMPILER_SEMANTIC_H