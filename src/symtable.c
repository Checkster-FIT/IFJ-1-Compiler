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

SymtableEntry* add_built_in_function(
    Symtable* symtable, char* identifier,
    const char* parameters, const char return_type, const bool extension)
{
    const SymtableEntry entry = {
        .function = true,
        .defined = true,
        .builtin = true,
        .extension = extension,
        .constant = true,
        .main = false,
        .identifier = identifier,
        .data_type = return_type,
        .parameters = NULL,
        .status = ANALYSIS_NONE,
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
    if (!add_built_in_function(symtable, "print", "#a", 'u', false))
        return NULL;
    if (!add_built_in_function(symtable, "readLine", "#", 's', false))
        return NULL;
    if (!add_built_in_function(symtable, "readInt", "#", 'i', false))
        return NULL;
    if (!add_built_in_function(symtable, "readDouble", "#", 'd', false))
        return NULL;
    if (!add_built_in_function(symtable, "toStr", "#a", 's', false))
        return NULL;
    if (!add_built_in_function(symtable, "toInt", "#d", 'i', false))
        return NULL;
    if (!add_built_in_function(symtable, "strLength", "#s", 'i', false))
        return NULL;
    if (!add_built_in_function(symtable, "subStr", "#sii", 's', false))
        return NULL;
    if (!add_built_in_function(symtable, "ord", "#si", 'i', false))
        return NULL;
    if (!add_built_in_function(symtable, "chr", "#i", 's', false))
        return NULL;

    // BOOL-THEN
    if (!add_built_in_function(symtable, "readBoolean", "#", 'b', false))
        return NULL;

    // VAL-MEM
    if (!add_built_in_function(symtable, "a#toString", "", 's', true))
        return NULL;
    if (!add_built_in_function(symtable, "d#toInt", "", 'i', true))
        return NULL;
    if (!add_built_in_function(symtable, "d#floor", "", 'd', true))
        return NULL;
    if (!add_built_in_function(symtable, "s#length", "", 'i', true))
        return NULL;

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
        return NULL;
    }

    *data = entry;
    data->status = ANALYSIS_NONE;

    if (data->parameters != NULL) {
        data->parameters = init_dynamic_string();
        if (data->parameters == NULL) {
            return NULL;
        }
    }

    // Redefinition check
    data->error = false;
    const TNode* result = search(symtable, entry.identifier);
    if (result != NULL && ((SymtableEntry*)result->value)->defined) {
        data->error = true;
        return data;
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
