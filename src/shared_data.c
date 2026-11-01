/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    shared_data.c
 * @brief   Implements functionality for storing and accessing shared data across modules
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "shared_data.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>

int init_shared_data(SharedData* data)
{
    data->line = 1;

    data->identifiers.ptr = malloc(sizeof(char) * IDENTIFIERS_BASE_CAPACITY);
    data->identifiers.char_capacity = IDENTIFIERS_BASE_CAPACITY;
    data->identifiers.char_length = 0;
    data->identifiers.count = 0;

    if (data->identifiers.ptr == NULL) {
        return print_error(MEMORY_ALLOCATION_ERROR, data->line);
    }

    data->strings.ptr = malloc(sizeof(char) * STRINGS_BASE_CAPACITY);
    data->strings.char_capacity = STRINGS_BASE_CAPACITY;
    data->strings.char_length = 0;
    data->strings.count = 0;

    if (data->strings.ptr == NULL) {
        free(data->identifiers.ptr);
        return print_error(MEMORY_ALLOCATION_ERROR, data->line);
    }

    return 0;
}

void destroy_shared_data(const SharedData* data)
{
    free(data->identifiers.ptr);
    free(data->strings.ptr);
}

int dynamic_add(DynamicString* dynamic_string, const char* str)
{
    const int idx = dynamic_string->char_length + 1;
    const int str_len = (int)strlen(str) + 1; // With '\0'

    // Increase the identifier string size if necessary
    if (dynamic_string->char_length + str_len >= dynamic_string->char_capacity) {

        const int new_capacity = (dynamic_string->char_capacity * 2) + str_len;
        char* ptr = realloc(dynamic_string->ptr, new_capacity);

        if (ptr == NULL)
            return -1;

        dynamic_string->ptr = ptr;
        dynamic_string->char_capacity = new_capacity;
    }

    dynamic_string->char_length += str_len;
    dynamic_string->ptr[idx - 1] = '\0';
    strcpy(&dynamic_string->ptr[idx], str);

    dynamic_string->count++;
    return idx;
}

int add_identifier(SharedData* data, const char* identifier)
{
    return dynamic_add(&data->identifiers, identifier);
}

char* get_identifier(const SharedData* data, const int offset)
{
    if (offset >= data->identifiers.char_length || offset < 0)
        return NULL;

    return &data->identifiers.ptr[offset];
}

int add_string(SharedData* data, const char* string)
{
    return dynamic_add(&data->strings, string);
}

char* get_string(const SharedData* data, const int offset)
{
    if (offset >= data->strings.char_length || offset < 0)
        return NULL;

    return &data->strings.ptr[offset];
}
