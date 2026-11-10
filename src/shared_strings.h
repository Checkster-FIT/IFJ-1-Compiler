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

AllocatedStrings* new_strings();
void free_strings(AllocatedStrings* strings);
void free_allocated_strings(const TNode* node);

char* add_string(const AllocatedStrings* strings, const char* str);

#endif // IFJ_1_COMPILER_STRINGS_H
