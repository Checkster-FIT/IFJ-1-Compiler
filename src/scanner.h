/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    scanner.h
 * @brief
 * @author
 */

#ifndef IFJ_1_COMPILER_SCANNER_H
#define IFJ_1_COMPILER_SCANNER_H

typedef struct
{
    enum {
        NONE,
        ERROR,
        INTEGER,
        DOUBLE
    } type;

    union {
        int attribute_int;
        double attribute_double;
        char* attribute_str_ptr;
    };
} Token;

Token get_token();

enum {
    H
} type;

#endif // IFJ_1_COMPILER_SCANNER_H