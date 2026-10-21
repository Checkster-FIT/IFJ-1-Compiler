/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    shared_data.h
 * @brief   Implements functionality for storing and accessing shared data across modules
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_COMPILER_H
#define IFJ_1_COMPILER_COMPILER_H

#define IDENTIFIERS_BASE_CAPACITY 128
#define STRINGS_BASE_CAPACITY 64

typedef struct {
    char* ptr;
    int char_length;
    int char_capacity;
    int count;
} DynamicString;

typedef struct {
    DynamicString identifiers;
    DynamicString strings;
} SharedData;

/**
 * Initializes shared data and allocates necessary memory
 * @param data the shared data structure to be initialized
 * @return 0 on success, non zero value on memory allocation error
 */
int init_shared_data(SharedData* data);

/** Frees up allocated memory used by shared data
 * @param data the shared data to be deallocated
 */
void destroy_shared_data(const SharedData* data);

/**
 * Adds a string at the end of a dynamic string, allocates memory if necessary
 * @param dynamic_string the dynamic string structure
 * @param str the string to be appended
 * @return the index of the string on success, -1 otherwise
 */
int dynamic_add(DynamicString* dynamic_string, const char* str);

/**
 * Adds a program identifier at the end of the identifier dynamic string,
 * allocates memory if necessary
 * @param data the shared data structure where the identifier will be stored
 * @param identifier the identifier string to be added
 * @return the index of the identifier on success, -1 otherwise
 */
int add_identifier(SharedData* data, const char* identifier);

/**
 * Gets a program identifier string pointer by offset index inside
 * a given shared data structure
 * @param data the shared data structure that contains the fetched string
 * @param offset the char offset index inside the data structure
 * representing the start of the fetched identifier string
 * @return the pointer to the string on success, NULL pointer if outside of a valid range
 */
char* get_identifier(const SharedData* data, int offset);

/**
 * Adds a program string at the end of the program string dynamic string,
 * allocates memory if necessary
 * @param data the shared data structure where the string will be stored
 * @param string the program string to be added
 * @return the index of the string on success, -1 otherwise
 */
int add_string(SharedData* data, const char* string);

/**
 * Gets a program string pointer by offset index inside
 * a given shared data structure
 * @param data the shared data structure that contains the fetched string
 * @param offset the char offset index inside the data structure
 * representing the start of the fetched program string
 * @return the pointer to the string on success, NULL pointer if outside of a valid range
 */
char* get_string(const SharedData* data, int offset);

#endif // IFJ_1_COMPILER_COMPILER_H
