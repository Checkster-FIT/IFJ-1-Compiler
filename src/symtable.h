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
    bool main;
    const char* identifier;
    char data_type;
    bool constant;
} SymtableEntry;

typedef RBTree Symtable;

Symtable* new_symtable();
void free_symtable(const Symtable* symtable);
void free_allocated_entries(const TNode* node);

SymtableEntry* symtable_add_entry(Symtable* symtable, SymtableEntry function);
SymtableEntry* symtable_add_var(Symtable* symtable, SymtableEntry var);

#endif // IFJ_1_COMPILER_SYMTABLE_H