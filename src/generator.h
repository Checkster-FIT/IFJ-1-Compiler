/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator.h
 * @brief   Generates '.ifj26code' from the final AST and the included symtables
 * @author  Tobias Mikan (xmikant00)
 */

#ifndef IFJ_1_COMPILER_GENERATOR_H
#define IFJ_1_COMPILER_GENERATOR_H

#include "ast.h"
#include "symtable.h"

void generate_program(ASTNode* root);

void generate_function(ASTNode* root);

void generate_nested_local_variables(ASTNode* node);

void generate_local_variables(Symtable* root);

void generate_parameters(ASTNodeList* parameter_group);

void generate_arguments(ASTNodeList* arguments);

void generate_expression(ASTNode* expression);

void generate_statements(ASTNode* statement, bool is_function_body);

bool is_string_expr(ASTNode* node);

void to_bool();

void to_str();

#endif // IFJ_1_COMPILER_GENERATOR_H
