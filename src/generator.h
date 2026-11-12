/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator.h
 * @brief
 * @author
 */

#ifndef IFJ_1_COMPILER_GENERATOR_H
#define IFJ_1_COMPILER_GENERATOR_H

#include "ast.h"
#include "symtable.h"

int run_generator(ASTNode* root);

void generate_program(ASTNode* root);

void generate_function(ASTNode* root);

void generate_local_variables(Symtable* root);

void generate_parameters(ASTNode* parameter);

void generate_expresssion(ASTNode* expression);

void generate_statements(ASTNode* statement);

#endif // IFJ_1_COMPILER_GENERATOR_H
