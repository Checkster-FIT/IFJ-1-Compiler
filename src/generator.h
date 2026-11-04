/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator.h
 * @brief
 * @author
 */

#ifndef IFJ_1_COMPILER_GENERATOR_H
#define IFJ_1_COMPILER_GENERATOR_H

#include "ast.h"
#include "shared_data.h"

int run_generator(ASTNode* ast, const SharedData* data);

#endif // IFJ_1_COMPILER_GENERATOR_H
