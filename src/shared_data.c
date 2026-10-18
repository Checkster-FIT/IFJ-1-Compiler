/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    shared_data.c
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "shared_data.h"
#include "shared.h"
#include <stdlib.h>
#include <string.h>

int dynamic_add(DynamicString* dynamic_string, const char* str)
{
    const int idx = dynamic_string->length + 1;
    const int str_len = (int)strlen(str) + 1; // With '\0'

    // Increase the identifier string size if necessary
    if (dynamic_string->length + str_len >= dynamic_string->capacity) {

        const int new_capacity = (dynamic_string->capacity * 2) + str_len;
        char* ptr = realloc(dynamic_string->ptr, new_capacity);

        if (ptr == NULL)
            return -1;

        dynamic_string->ptr = ptr;
        dynamic_string->capacity = new_capacity;
    }

    dynamic_string->length += str_len;
    dynamic_string->ptr[idx - 1] = '\0';
    strcpy(&dynamic_string->ptr[idx], str);
    return idx;
}

int add_identifier(SharedData* data, const char* identifier)
{
    return dynamic_add(&data->identifiers, identifier);
}

char* get_identifier(const SharedData* data, const int offset)
{
    return &data->identifiers.ptr[offset];
}

int add_string(SharedData* data, const char* string)
{
    return dynamic_add(&data->strings, string);
}

char* get_string(const SharedData* data, const int offset)
{
    return &data->strings.ptr[offset];
}

int init_shared_data(SharedData* data)
{
    data->identifiers.ptr = malloc(sizeof(char) * IDENTIFIERS_BASE_CAPACITY);
    data->identifiers.length = 0;
    data->identifiers.capacity = IDENTIFIERS_BASE_CAPACITY;

    if (data->identifiers.ptr == NULL) {
        return print_error(MEMORY_ALLOCATION_ERROR);
    }

    data->strings.ptr = malloc(sizeof(char) * STRINGS_BASE_CAPACITY);
    data->strings.length = 0;
    data->strings.capacity = STRINGS_BASE_CAPACITY;

    if (data->strings.ptr == NULL) {
        free(data->identifiers.ptr);
        return print_error(MEMORY_ALLOCATION_ERROR);
    }

    return 0;
}