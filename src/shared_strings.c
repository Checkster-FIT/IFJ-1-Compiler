/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    shared_strings.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "shared_strings.h"
#include "error.h"

#include <string.h>

AllocatedStrings *new_strings() {
    AllocatedStrings *identifiers = malloc(sizeof(AllocatedStrings));

    if (identifiers == NULL)
        print_error(MEMORY_ALLOCATION_ERROR, 0);

    identifiers->tree = tree_init(rb_tree_strcmp);

    if (identifiers->tree == NULL)
        print_error(MEMORY_ALLOCATION_ERROR, 0);

    return identifiers;
}

void free_strings(AllocatedStrings *strings) {
    free_allocated_strings(strings->tree->root);
    free(strings->tree);
    free(strings);
}

void free_allocated_strings(const TNode *node) {
    if (node == NULL)
        return;

    free_allocated_strings(node->left);
    free_allocated_strings(node->right);
    free(node->value);
    free((void *) node);
}

char *add_string(const AllocatedStrings *strings, const char *str) {
    const size_t length = strlen(str) + 1;

    const TNode *node = search(strings->tree, str);

    if (node != NULL)
        return node->value; // String already exists

    char *
    
    new = malloc(length);
    if (new == NULL)
        return NULL;

    strncpy(new, str, length);
    node = tree_insert(strings->tree, new, new);

    if (node == NULL) {
        free(new);
        return NULL;
    }

    return new;
}

// ====================================

DynamicString *init_dynamic_string() {
    const size_t string_len = 64;

    DynamicString *string_struct = malloc(sizeof(DynamicString));

    if (!string_struct) {
        return NULL;
    }

    char *string_chars = malloc(sizeof(char) * string_len);

    if (!string_chars) {
        free(string_struct);
        return NULL;
    }

    string_struct->chars = string_chars;
    string_struct->chars[0] = '\0';
    string_struct->len = 0;
    string_struct->capacity = string_len;

    return string_struct;
}

int resize_dynamic_string(DynamicString *string) {
    const size_t new_capacity = string->capacity * 2;
    char *new_string = realloc(string->chars, sizeof(char) * new_capacity);
    if (!new_string) {
        // realloc error
        return 1;
    }

    string->capacity = new_capacity;
    string->chars = new_string;

    return 0;
}

void clear_dynamic_string(DynamicString *string) {
    string->len = 0;
    string->chars[0] = '\0';
}

int dynamic_string_append(DynamicString *string, char c) {
    if (string->len + 1 >= string->capacity) {
        int result = resize_dynamic_string(string);
        if (result == 1)
            return 1;
    }

    string->chars[string->len] = c;
    string->len += 1;
    string->chars[string->len] = '\0';

    return 0;
}

int dynamic_string_append_str(DynamicString *string, const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (dynamic_string_append(string, str[i]))
            return 1;
    }

    return 0;
}

void dynamic_string_remove_last(DynamicString *string) {
    if (string->len == 0)
        return; // nothing to be removed

    string->len -= 1;
    string->chars[string->len] = '\0';
}

char dynamic_string_top(const DynamicString *string) {
    // empty string
    if (string->len == 0)
        return '\0';

    return string->chars[(string->len) - 1];
}

void dispose_dynamic_string(DynamicString *string) {
    if (!string)
        return;
    free(string->chars);
    free(string);
}