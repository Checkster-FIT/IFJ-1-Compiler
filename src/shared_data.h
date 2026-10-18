/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    shared_data.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_COMPILER_H
#define IFJ_1_COMPILER_COMPILER_H

#define IDENTIFIERS_BASE_CAPACITY 2
#define STRINGS_BASE_CAPACITY 2
#include <stddef.h>

typedef struct {
    char* ptr;
    int length;
    int capacity;
} DynamicString;

typedef struct {
    DynamicString identifiers;
    DynamicString strings;
} SharedData;

int init_shared_data(SharedData* data);
int add_identifier(SharedData* data, const char* identifier);
char* get_identifier(const SharedData* data, int offset);
int add_string(SharedData* data, const char* string);
char* get_string(const SharedData* data, int offset);

#endif // IFJ_1_COMPILER_COMPILER_H
