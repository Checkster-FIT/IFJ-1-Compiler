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

SymtableEntry* add_built_in_function(Symtable* symtable, char* identifier, const char* parameters, const char return_type)
{
    const SymtableEntry entry = {
        .function = true,
        .defined = true,
        .builtin = true,
        .constant = true,
        .main = false,
        .identifier = identifier,
        .data_type = return_type,
        .parameters = NULL,
    };

    SymtableEntry* added_entry = symtable_add_entry(symtable, entry);
    if (!added_entry)
        return NULL;

    added_entry->parameters = init_dynamic_string();
    if (!added_entry->parameters)
        return NULL;

    if (dynamic_string_append_str(added_entry->parameters, parameters) == 1)
        return NULL;

    return added_entry;
}

Symtable* add_built_in_entries(Symtable* symtable)
{
    add_built_in_function(symtable, "print", "#a", 'u');
    add_built_in_function(symtable, "readLine", "#", 's');
    add_built_in_function(symtable, "readInt", "#", 'i');
    add_built_in_function(symtable, "readDouble", "#", 'd');
    add_built_in_function(symtable, "toStr", "#a", 's');
    add_built_in_function(symtable, "toInt", "#d", 'i');
    add_built_in_function(symtable, "strLength", "#s", 'i');
    add_built_in_function(symtable, "subStr", "#sii", 'i');
    add_built_in_function(symtable, "ord", "#si", 'i');
    add_built_in_function(symtable, "chr", "#i", 's');

    // BOOL-THEN
    add_built_in_function(symtable, "readBoolean", "#", 'b');

    // VAL-MEM
    add_built_in_function(symtable, "floor", "", 'd');
    add_built_in_function(symtable, "toString", "", 's');
    add_built_in_function(symtable, "length", "", 'i');
    // add_built_in_function(symtable, "toInt", "", 'i'); // ?????

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

    const SymtableEntry* entry = node->value;
    if (entry != NULL) {
        dispose_dynamic_string(entry->parameters);
        free(node->value);
    }

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

    if (data->parameters != NULL) {
        data->parameters = init_dynamic_string();
        if (data->parameters == NULL) {
            print_error(MEMORY_ALLOCATION_ERROR, -1);
            return NULL;
        }
    }

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
