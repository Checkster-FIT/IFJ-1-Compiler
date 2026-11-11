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

Symtable* add_builtin_entries(Symtable* symtable)
{
    SymtableEntry entry = {
        .function = true,
        .identifier = "",
        .data_type = '?',
        .defined = true,
        .main = false,
    };

    entry.identifier = "print";
    entry.data_type = 'u';
    symtable_add_entry(symtable, entry);

    entry.identifier = "readLine";
    entry.data_type = 's';
    symtable_add_entry(symtable, entry);

    entry.identifier = "readInt";
    entry.data_type = 'i';
    symtable_add_entry(symtable, entry);

    entry.identifier = "readDouble";
    entry.data_type = 'd';
    symtable_add_entry(symtable, entry);

    entry.identifier = "toStr";
    entry.data_type = 's';
    symtable_add_entry(symtable, entry);

    entry.identifier = "toInt";
    entry.data_type = 'i';
    symtable_add_entry(symtable, entry);

    entry.identifier = "strLength";
    entry.data_type = 'i';
    symtable_add_entry(symtable, entry);

    entry.identifier = "subStr";
    entry.data_type = 'i';
    symtable_add_entry(symtable, entry);

    entry.identifier = "ord";
    entry.data_type = 'i';
    symtable_add_entry(symtable, entry);

    entry.identifier = "chr";
    entry.data_type = 's';
    symtable_add_entry(symtable, entry);

    // BOOL-THEN
    entry.identifier = "readBoolean";
    entry.data_type = 'b';
    symtable_add_entry(symtable, entry);

    // VAL-MEM
    entry.identifier = "floor";
    entry.data_type = 'd';
    symtable_add_entry(symtable, entry);

    entry.identifier = "toString";
    entry.data_type = 's';
    symtable_add_entry(symtable, entry);

    // entry.identifier = "toInt"; // ?????
    // entry.data_type = 'i';
    // symtable_add_entry(symtable, entry);

    entry.identifier = "length";
    entry.data_type = 'i';
    symtable_add_entry(symtable, entry);

    return symtable;
}

void free_symtable(const Symtable* symtable)
{
    if (symtable == NULL) {
        return;
    }
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

SymtableEntry* symtable_add_entry(Symtable* symtable, const SymtableEntry entry)
{
    SymtableEntry* data = malloc(sizeof(SymtableEntry));

    if (data == NULL) {
        print_error(MEMORY_ALLOCATION_ERROR, -1);
        return NULL;
    }

    *data = entry;

    if (tree_insert(symtable, entry.identifier, data) == NULL)
        return NULL;

    const TNode* node = search(symtable, entry.identifier);
    return node->value;
}

static SymtableEntry* find_main(const TNode* node)
{
    if (node == NULL)
        return NULL;

    SymtableEntry* entry = node->value;
    if (entry != NULL && entry->main)
        return entry;

    SymtableEntry* found = find_main(node->left);
    if (found != NULL)
        return found;

    return find_main(node->right);
}

SymtableEntry* symtable_main_search(Symtable* symtable)
{
    if (symtable == NULL)
        return NULL;

    return find_main(symtable->root);
}
