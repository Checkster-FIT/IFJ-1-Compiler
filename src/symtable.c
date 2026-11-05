/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    symtable.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "symtable.h"
#include "bst.h"
#include "error.h"
#include <stdbool.h>

Symtable* create_symtable()
{
    Symtable* ptr = tree_init(rb_tree_strcmp);

    if (ptr == NULL)
        print_error(MEMORY_ALLOCATION_ERROR, 0);

    return ptr;
}

SymtableFunction* symtable_add_function(Symtable* symtable, const SymtableFunction function)
{
    SymtableFunction* data = malloc(sizeof(SymtableFunction));

    if (data == NULL)
        print_error(MEMORY_ALLOCATION_ERROR, -1);

    *data = function;
    const TNode* node = tree_insert(symtable, &data->index, data);

    return node->value;
}