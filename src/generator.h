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

int run_generator(ASTNode* ast, const Symtable* symtable);

#endif // IFJ_1_COMPILER_GENERATOR_H
