/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    shared_strings.h
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_STRINGS_H
#define IFJ_1_COMPILER_STRINGS_H
#include "bst.h"

typedef struct {
    RBTree* tree;
} AllocatedStrings;

typedef struct {
    char* chars;
    size_t len;
    size_t capacity;
} DynamicString;

AllocatedStrings* new_strings();

void free_strings(AllocatedStrings* strings);

void free_allocated_strings(const TNode* node);

char* add_string_copy(const AllocatedStrings* strings, const char* str);

/**
 * @brief Allocates and initializes a dynamic string structure.
 *
 * @return Pointer to the allocated DynamicString, or NULL on malloc fail.
 */
DynamicString* init_dynamic_string();

/**
 * @brief Doubles the capacity of the string buffer.
 *
 * @param string Pointer to the dynamic string.
 * @return 0 on success, 1 on realloc fail.
 */
int resize_dynamic_string(DynamicString* string);

/**
 * @brief Empties the dynamic string (without freeing memory).
 *
 * @param string Pointer to the dynamic string.
 */
void clear_dynamic_string(DynamicString* string);

/**
 * @brief Appends character to the string, resizing if necessary.
 * @param string Pointer to the dynamic string.
 * @param c Character to append.
 * @return 0 on success, 1 on memory allocation fail.
 */
int dynamic_string_append(DynamicString* string, char c);

/**
 * @brief Appends another string at the end of the dynamic string, resizing if necessary.
 * @param string Pointer to the dynamic string.
 * @param str String to append.
 * @return 0 on success, 1 on memory allocation fail.
 */
int dynamic_string_append_str(DynamicString* string, const char* str);

/**
 * @brief Removes the last character from the string.
 *
 * @param string Pointer to the dynamic string.
 */
void dynamic_string_remove_last(DynamicString* string);

/**
 * @brief Returns the last character in the string.
 *
 * @param string Pointer to the dynamic string.
 * @return The last character, or '\0' if the string is empty.
 */
char dynamic_string_top(const DynamicString* string);

/**
 * @brief Frees the string structure and its buffer.
 *
 * @param string Pointer to the dynamic string.
 */
void dispose_dynamic_string(DynamicString* string);

#endif // IFJ_1_COMPILER_STRINGS_H