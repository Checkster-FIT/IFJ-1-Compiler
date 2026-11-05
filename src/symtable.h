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
    bool defined;
    int index;
    char type;
} SymtableEntry;

typedef RBTree Symtable;

Symtable* create_symtable();
SymtableEntry* symtable_add(Symtable* symtable, SymtableEntry function);
SymtableEntry* symtable_add_var(Symtable* symtable, SymtableEntry var);

#endif // IFJ_1_COMPILER_SYMTABLE_H