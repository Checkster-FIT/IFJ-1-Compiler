/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    error.c
 * @brief   Contains error handling logic used across modules
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

ErrorMessage get_error_by_id(const enum error_enum error_id)
{
    ErrorMessage error;
    error.id = error_id;
    error.fix = "";

    // [!] Add more error messages here
    // - Error messages should include a brief explanation of the error (.message)
    // - Error messages should also include a potential fix for the error (.fix)
    // - Exit code should correspond to the error type, see below + project specification
    // [!] Usage example:
    //   return print_error(SCANNER_INT_LEADING_ZERO, "0195", 195);
    switch (error.id) {
        case SCANNER_NO_TRANSITION:
            error.message = "No scanner transition was found for state '%i' and lexeme %s";
            error.fix = "Try adding this case in scanner.c";
            error.code = 100;
            break;
        case SCANNER_INT_LEADING_ZERO:
            error.message = "Non-zero integers cannot contain leading zeros: '%s'";
            error.fix = "Integer literal can be replaced with a valid '%i' equivalent";
            error.code = 1;
            break;
        case SCANNER_NUM_INVALID_UNDERSCORE:
            error.message = "Underscore(s) inside an integer can only be used between two numbers: '%s' is incorrect";
            error.fix = "Remove underscore(s) or put numbers around them";
            error.code = 1;
            break;
        case SCANNER_DOUBLE_INVALID_DECIMAL_POINT:
            error.message = "Decimal point must be followed by a number: '%s'";
            error.fix = "Add a number right after the decimal point";
            error.code = 1;
            break;
        case SCANNER_DOUBLE_MISSING_EXPONENT:
            error.message = "Missing exponent of Double: '%s'; e/E (may be followed by a sign) must be followed by a number";
            error.fix = "Remove the e/E or add an exponent";
            error.code = 1;
            break;
        case SCANNER_UNKNOWN_OPERATOR:
            error.message = "An unknown operator '%s' was found";
            error.fix = "Did you mean to use '%s'?";
            error.code = 1;
            break;
        case SCANNER_INVALID_STRING:
            error.message = "A string cannot contain '%s': '%s'";
            error.fix = "End the string properly first";
            error.code = 1;
            break;
        case SCANNER_INVALID_BLOCK_COMMENT:
            error.message = "A block comment needs to be ended before end of the file";
            error.code = 1;
            break;
        case SCANNER_INVALID_IDENTIFIER:
            error.message = "'%s' is not a valid identifier";
            error.fix = "An identifier can only contain letters, digits, and underscore. It begins with a letter or an underscore. The underscore itself is not a valid identifier";
            error.code = 1;
            break;
        case SCANNER_BLOCK_COMMENT_FOLLOWED_BY_CHARS:
            error.message = "A block comment cannot be followed by non-whitespace characters on the same line: '%s'";
            error.fix = "Remove the characters between the block comment and end of line";
            error.code = 1;
            break;
        case SYNTAX_INCORRECT_USAGE_OF_AT_SYMBOL:
            error.message = "The symbol @ can only be followed by identifier 'main'";
            error.code = 2;
        case MEMORY_ALLOCATION_ERROR:
            error.message = "An error has occurred when trying to allocate memory for '%s'";
            error.code = 99;
            break;
        case STREAM_READ_ERROR:
            error.message = "An error has occurred when trying to read from the input stream";
            error.code = 99;
            break;
        case CORRUPTED_MEMORY:
            error.message = "An error has occurred when trying to read from memory in state: '%i'";
            error.code = 99;
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
    if (exit_code == 100)
        return "Program error";

    return "Unknown error";
}

int print_error(const int error_id, ...)
{
    // Gets the full error message for the error with this id
    const ErrorMessage error = get_error_by_id(error_id);

    // Build the error message string format from its components
    char error_message[1024];

    // Create the format with or without the fix
    const char* format = "\033[1;31m[%s]: %s\n\033[0m -> %s\n";
    if (strlen(error.fix) <= 0)
        format = "\033[1;31m[%s]: %s\n";

    snprintf(
        error_message,
        sizeof(error_message),
        format,
        get_error_type_str(error.code), error.message, error.fix);

    // Print the constructed format with the user specified arguments
    va_list args = { };
    va_start(args, error_id);
    vfprintf(stderr, error_message, args);
    va_end(args);

    return error.code;
}