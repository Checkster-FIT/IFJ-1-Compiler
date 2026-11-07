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

void generator(ASTNode* root);

void generate_program(ASTNode* root);

void generate_function(ASTNode* root);

void print_call(const char* name);

void print_label(const char* name);

#endif // IFJ_1_COMPILER_GENERATOR_H
