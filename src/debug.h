/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    debug.h
 * @brief   Various debug and logging functionality
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_DEBUG_H
#define IFJ_1_COMPILER_DEBUG_H

#include "scanner.h"
#include "shared_data.h"

void print_token(const Token* token, const SharedData* data);

#endif // IFJ_1_COMPILER_DEBUG_H
