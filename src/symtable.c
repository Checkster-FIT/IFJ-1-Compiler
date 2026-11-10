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

Symtable* new_symtable()
{
    Symtable* ptr = tree_init(rb_tree_strcmp);

    if (ptr == NULL)
        print_error(MEMORY_ALLOCATION_ERROR, 0);

    return ptr;
}

void free_symtable(const Symtable* symtable)
{
    free_allocated_entries(symtable->root);
    free((RBTree*)symtable);
}

void free_allocated_entries(const TNode* node)
{
    if (node == NULL)
        return;

    free_allocated_entries(node->left);
    free_allocated_entries(node->right);
    free(node->value);
    free((void*)node);
}

SymtableEntry* symtable_add(Symtable* symtable, const SymtableEntry function)
{
    SymtableEntry* data = malloc(sizeof(SymtableEntry));

    if (data == NULL) {
        print_error(MEMORY_ALLOCATION_ERROR, -1);
        return NULL;
    }

    *data = function;
    const TNode* node = tree_insert(symtable, data->identifier, data);

    return node->value;
}
