/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    symtable.h
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_SYMTABLE_H
#define IFJ_1_COMPILER_SYMTABLE_H

#include "bst.h"
#include <stdbool.h>

typedef struct {
    char* parameters;
    bool defined;
    int index;
} SymtableFunction;

typedef RBTree Symtable;

Symtable* create_symtable();
SymtableFunction* symtable_add_function(Symtable* symtable, SymtableFunction function);

#endif // IFJ_1_COMPILER_SYMTABLE_H