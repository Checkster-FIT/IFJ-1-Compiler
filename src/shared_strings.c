/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    shared_strings.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "shared_strings.h"
#include "error.h"

#include <string.h>

AllocatedStrings* new_strings()
{
    AllocatedStrings* identifiers = malloc(sizeof(AllocatedStrings));

    if (identifiers == NULL)
        print_error(MEMORY_ALLOCATION_ERROR, 0);

    identifiers->tree = tree_init(rb_tree_strcmp);

    if (identifiers->tree == NULL)
        print_error(MEMORY_ALLOCATION_ERROR, 0);

    return identifiers;
}

void free_strings(AllocatedStrings* strings)
{
    (void)strings;
    return;
}

char* add_string(const AllocatedStrings* strings, const char* str)
{
    const size_t length = strlen(str) + 1;

    char* new = malloc(length);
    if (new == NULL)
        return NULL;

    strncpy(new, str, length);
    const TNode* node = tree_insert(strings->tree, new, new);

    if (node == NULL) {
        free(new);
        return NULL;
    }

    return new;
}

// DynamicString* create_dynamic_string(const char* str)
// {
//     const size_t length = strlen(str) + 1;
//
//     DynamicString* object = malloc(sizeof(DynamicString));
//     object->ptr = malloc(length);
//
//     strncpy(object->ptr, str, length);
//     return object;
// }


