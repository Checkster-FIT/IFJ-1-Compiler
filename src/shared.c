/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    shared.c
 * @brief   Contains shared logic used across modules (error handling, ...)
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "shared.h"
#include <stdarg.h>
#include <stdio.h>

ErrorMessage get_error_by_id(const enum error_enum error_id)
{
    ErrorMessage error;
    error.id = error_id;

    // [!] Add more error messages here
    // - Error messages should include a brief explanation of the error (.message)
    // - Error messages should also include a potential fix for the error (.fix)
    // - Exit code should correspond to the error type, see below + project specification
    // [!] Usage example:
    //   return print_error(SCANNER_INT_LEADING_ZERO, "0195", 195);
    switch (error.id) {
        case SCANNER_INT_LEADING_ZERO:
            error.message = "Non-zero integers cannot contain leading zeros: '%s'";
            error.fix = "Integer literal can be replaced with a valid '%i' equivalent";
            error.code = 1;
            break;
        default:
            error.message = "An unknown error has occurred...?";
            error.fix = "Try adding a valid error message in the shared module";
            error.code = 100;
            break;
    }

    return error;
}

char* get_error_type_str(const int exit_code)
{
    if (exit_code == 1)
        return "Lexical error";
    if (exit_code == 2)
        return "Syntax error";
    if ((exit_code >= 3 && exit_code <= 7) || (exit_code == 10))
        return "Semantic error";
    if (exit_code == 99)
        return "Internal error";

    return "Unknown error";
}

int print_error(const int error_id, ...)
{
    // Gets the full error message for the error with this id
    const ErrorMessage error = get_error_by_id(error_id);

    // Build the error message string format from its components
    char format[1024];
    snprintf(
        format,
        sizeof(format),
        "\033[1;31m[%s]: %s\n\033[0m -> %s\n",
        get_error_type_str(error.code), error.message, error.fix);

    // Print the constructed format with the user specified arguments
    va_list args;
    va_start(args, error_id);
    vfprintf(stderr, format, args);
    va_end(args);

    return error.code;
}