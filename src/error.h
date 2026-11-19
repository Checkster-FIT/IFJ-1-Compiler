/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    error.h
 * @brief   Contains error handling logic used across modules
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_ERROR_H
#define IFJ_1_COMPILER_ERROR_H
#include <stdarg.h>
#include <stdio.h>

#define EXIT_CODE_INTERNAL_ERROR (99)

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
    SCANNER_INVALID_INDENTATION,
    SCANNER_INVALID_INDENTATION_USING_TAB,

    // Parser error
    PARSER_SYNTAX_ERROR,
    PARSER_GLOBAL_SCOPE_INVALID,
    PARSER_MISSING_HEADER,
    PARSER_MISSING_IMPORT,
    PARSER_INVALID_FUNCTION,
    PARSER_FUNCTION_MISSING_INDENT,
    PARSER_FUNCTION_SINGLE_LINE_MISSING_LINE_FEED,
    PARSER_ST_VAR_DEF_MISSING_IDENTIFIER,
    PARSER_ST_VAR_DEF_MISSING_ASSIGNMENT,
    PARSER_ST_MISSING_SEPARATOR,
    PARSER_MISSING_EXT_PARAM,
    PARSER_MISSING_EXT_PARAM_END,
    PARSER_EXT_MISSING_NEWLINE,
    PARSER_EXT_MISSING_INDENT,
    PARSER_EXT_BLOCK_INVALID,
    PARSER_MISSING_PARAM_BRACKET,
    PARSER_MISSING_PARAM_TYPE,
    PARSER_INCORRECT_TYPE,
    PARSER_INVALID_PARAM_LIST_END,
    PARSER_DOUBLE_PARAM_TYPE,
    PARSER_INVALID_PARAM,
    PARSER_MISSING_FUNCTION_ASSIGNMENT,
    PARSER_WHILE_MISSING_DO,
    PARSER_WHILE_MISSING_LINE_FEED,
    PARSER_WHILE_MISSING_INDENT,
    PARSER_IF_MISSING_THEN,
    PARSER_IF_MISSING_INDENT,
    PARSER_IF_MISSING_DEDENT,
    PARSER_COND_STATEMENT_UNCLOSED,
    PARSER_INVALID_EXPRESSION_USE,
    PARSER_INVALID_EXPRESSION_FORMAT,
    PARSER_INVALID_FUNCTION_DEF,
    PARSER_MISSING_PARAM_IDENTIFIER,
    PARSER_FUNCTION_MISSING_BODY,
    PARSER_MULTIPLE_IMPORT_HEADERS,

    PARSER_RULE_INCOMPLETE,
    PARSER_RULE_INCORRECT_CALL,

    // Syntax error
    SYNTAX_INCORRECT_USAGE_OF_AT_SYMBOL,

    // Semantic error
    SEMANTIC_INVALID_REDEFINITION,
    SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE,
    SEMANTIC_MULTIPLE_MAIN,
    SEMANTIC_NO_MAIN,
    SEMANTIC_VARIABLE_OR_FUNCTION_REDEFINITION,
    SEMANTIC_NEGATION_OF_NON_BOOL_TYPE,
    SEMANTIC_UNARY_OPERATION_NOT_OVER_NUMBER,
    SEMANTIC_NUMERIC_OPERATION_NOT_OVER_NUMBERS,
    SEMANTIC_NOT_OVER_NUMBERS_OR_STRINGS,
    SEMANTIC_EQUAL_NOT_EQUAL_TYPE_MISMATCH,
    SEMANTIC_BINARY_OP_NOT_BOOL,
    SEMANTIC_IF_WHILE_CONDITION_NOT_BOOL,
    SEMANTIC_NOT_MATCHING_VAR_TYPE,
    SEMANTIC_USE_OF_UNDEFINED_VAL_FUN,
    SEMANTIC_FUNCALL_PARAMETER_MISMATCH,
    SEMANTIC_FUNCALL_PARAMETER_TYPE_MISMATCH,
    SEMANTIC_ARGUMENT_PASSED_TO_VARIABLE,
    SEMANTIC_STANDALONE_RETURN,
    SEMANTIC_RETURN_NOT_ALLOWED,
    SEMANTIC_RETURN_IN_F_WITHOUT_RETURN_TYPE,
    SEMANTIC_MAIN_WITH_PARAMETERS,
    SEMANTIC_MAIN_NOT_UNIT,
    SEMANTIC_WRITE_INTO_CONST_OR_FUN,
    SEMANTIC_RECURSION_WITHOUT_TYPE_DEF,
    SEMANTIC_DOT_NOTATION_TYPE_MISMATCH,
    SEMANTIC_UNABLE_TO_DETERMINE_RETURN_TYPE,
    SEMANTIC_EXTENSION_INVALID_CALL_TYPE,

    // Internal error
    MEMORY_ALLOCATION_ERROR,
    STREAM_READ_ERROR,
    CORRUPTED_MEMORY,
    NUMBER_CONVERT_ERROR,
    SYMTABLE_ENTRY_NOT_FOUND,
} ErrorEnum;

typedef struct {
    ErrorEnum id;
    char* message;
    char* fix;
    int code;
    int param_count;
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
 * @param line the program line where the error occured
 * @param ... parameters for the formatted messages in the @c ErrorMessage struct,
 * each message requires different parameters (some error messages do not require any)
 * @return the exit code for the given error message
 */
int print_error(int error_id, int line, ...);

int va_print_error(int error_id, int line, va_list args);

/**
 * @brief Checks the range of exit codes and returns the corresponding error type
 * @param exit_code the error exit code
 * @return the error type string ("Syntax error", "Semantic error", ...)
 */
char* get_error_type_str(int exit_code);

#endif // IFJ_1_COMPILER_ERROR_H