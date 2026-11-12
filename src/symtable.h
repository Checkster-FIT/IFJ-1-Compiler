/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    symtable.h
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_SYMTABLE_H
#define IFJ_1_COMPILER_SYMTABLE_H

#include "bst.h"
#include "shared_strings.h"

#include <stdbool.h>

typedef struct {
    const char* identifier;
    char data_type;
    DynamicString* parameters;

    bool defined;
    bool builtin;
    bool function;
    bool constant;
    bool main;
} SymtableEntry;

typedef RBTree Symtable;

Symtable* new_symtable();

SymtableEntry* add_built_in_function(Symtable* symtable, char* identifier, const char* parameters, char return_type);
Symtable* add_built_in_entries(Symtable* symtable);

void free_symtable(const Symtable* symtable);
void free_allocated_entries(const TNode* node);

SymtableEntry* symtable_add_entry(Symtable* symtable, SymtableEntry entry);

#endif // IFJ_1_COMPILER_SYMTABLE_H