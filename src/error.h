/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    error.h
 * @brief   Contains error handling logic used across modules
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_SHARED_H
#define IFJ_1_COMPILER_SHARED_H

typedef enum error_enum {
    // Scanner error
    SCANNER_NO_TRANSITION,
    SCANNER_INT_LEADING_ZERO,
    SCANNER_NUM_INVALID_UNDERSCORE,
    SCANNER_DOUBLE_INVALID_DECIMAL_POINT,
    SCANNER_DOUBLE_MISSING_EXPONENT,
    SCANNER_UNKNOWN_OPERATOR,
    SCANNER_INVALID_STRING,
    SCANNER_INVALID_BLOCK_COMMENT,
    SCANNER_INVALID_IDENTIFIER,
    SCANNER_BLOCK_COMMENT_FOLLOWED_BY_CHARS,

    // Parser error

    // Internal error
    MEMORY_ALLOCATION_ERROR,
    STREAM_READ_ERROR,
    CORRUPTED_MEMORY,
    NUMBER_CONVERT_ERROR,
} ErrorEnum;

typedef struct {
    ErrorEnum id;
    char* message;
    char* fix;
    int code;
} ErrorMessage;

/**
 * @brief Gets the @c ErrorMessage struct corresponding to the given ID,
 * all error messages get added here and can be printed with @c print_error()
 * @param error_id and error index from the @c error_enum
 * @return an @c ErrorMessage struct with error messages and the exit code
 */
ErrorMessage get_error_by_id(enum error_enum error_id);

/**
 * @brief Prints and error message to @c stderr
 * @param error_id the error index from the @c error_enum
 * @param ... parameters for the formatted messages in the @c ErrorMessage struct,
 * each message requires different parameters (some error messages do not require any)
 * @return the exit code for the given error message
 */
int print_error(int error_id, ...);

/**
 * @brief Checks the range of exit codes and returns the corresponding error type
 * @param exit_code the error exit code
 * @return the error type string ("Syntax error", "Semantic error", ...)
 */
char* get_error_type_str(int exit_code);

#endif // IFJ_1_COMPILER_SHARED_H