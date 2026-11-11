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
    const char* identifier;
    bool defined;
    char data_type;
    bool function;
    bool constant;
    bool main;
} SymtableEntry;

typedef RBTree Symtable;

Symtable* new_symtable();
Symtable* add_builtin_entries(Symtable* symtable);
void free_symtable(const Symtable* symtable);
void free_allocated_entries(const TNode* node);

SymtableEntry* symtable_add(Symtable* symtable, SymtableEntry function);

SymtableEntry* symtable_add_entry(Symtable* symtable, SymtableEntry entry);

SymtableEntry* symtable_main_search(Symtable* symtable);

#endif // IFJ_1_COMPILER_SYMTABLE_H