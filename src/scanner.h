/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    scanner.h
 * @brief   Declaration of the Token type, get_token() and enumeration of the FSM states used to implement the scanner.
 * @author  Drahovska Andrea (xdrahoa00), Wyslych Vojtech (xwyslyv00)
 */

#ifndef IFJ_1_COMPILER_SCANNER_H
#define IFJ_1_COMPILER_SCANNER_H

#define RETURN_TOKEN_AND_SET_READ_BUFFER \
    context->read_buffer = true;         \
    return token;

#define IS_IDENTIFIER (-1)
#define IS_BOOLEAN_VALUE_TRUE 23
#define IS_BOOLEAN_VALUE_FALSE 24
#define NUMBER_OF_KEYWORDS 24

#include "error.h"
#include "shared_data.h"
#include <stdbool.h>
#include <stdio.h>

enum attribute_operation_type {
    PLUS, // +
    MINUS, // -
    MULTIPLY, // *
    AND, // &&
    OR, // ||
    NOT, // !
    DIVIDE, // /
};

enum attribute_keyword {
    DEF_KW,
    DO_KW,
    DOUBLE_KW,
    ELSE_KW,
    IF_KW,
    IMPORT_KW,
    INT_KW,
    RETURN_KW,
    STRING_KW,
    THEN_KW,
    UNIT_KW,
    VAL_KW,
    VAR_KW,
    WHILE_KW,
    BOOLEAN_KW,
    CASE_KW,
    CLASS_KW,
    EXTENSION_KW,
    FOR_KW,
    LIST_KW,
    MATCH_KW,
    TYPE_KW,
};

enum attribute_compare_type {
    EQUAL, // ==
    NOT_EQUAL, // !=
    LESS, // <
    GREATER, // >
    LESS_EQUAL, // <=
    GREATER_EQUAL, // >=
};

typedef struct token {
    enum type_enum {
        NONE,
        ERROR,
        INTEGER,
        DOUBLE,
        OPERATOR,
        IDENTIFIER,
        KEYWORD,
        BOOLEAN,
        STRING_INTERPOLATION,
        STRING_INTERPOLATION_END,
        MULTILINE_STRING_INTERPOLATION,
        MULTILINE_STRING_INTERPOLATION_END,
        STRING,
        MULTILINE_STRING,
        DOT,
        DECLARATION_SIGN,
        COMMA,
        INDENT,
        LINE_FEED,
        WHITESPACE,
        ASSIGNMENT,
        COMPARE,
        LEFT_BRACKET,
        RIGHT_BRACKET,
        END_OF_FILE
    } type;

    union {
        long attribute_int;
        double attribute_double;
        int attribute_str_idx;
        const char* attribute_str_ptr;
        bool attribute_bool;
        void* attribute_none;

        enum error_enum attribute_error;
        enum attribute_keyword attribute_keyword;
        enum attribute_operation_type attribute_operation_type;
        enum attribute_compare_type attribute_compare_type;
    };
} Token;

typedef enum {

    STATE_START, // initial state dispatcher

    // operators
    STATE_AMPERSAND_1, // expecting second '&' for AND operator
    STATE_OR_1, // expecting second '|' for OR operator
    STATE_OPERATOR, // operator complete, resolve type:  +, -, *, &&, ||
    STATE_EXCLAMATION_SYMBOL, // ! can be logical NOT or beginning of !=

    // strings
    STATE_STRING_WHICH, // select if standard string, empty string, or multiline string
    STATE_STRING_BEGIN, // standard string
    STATE_STRING_ESCAPE, // escape sequences "\"
    STATE_STRING_END, // end of the string
    STATE_EMPTY_STRING, // "", an empty string, or start of the multiline string

    // multiline string """
    STATE_STRING_MULTILINE_START, // """ start of the multiline string
    STATE_STRING_MULTILINE_END_1, // found first closing quote of multiline block
    STATE_STRING_MULTILINE_END_2, // found second closing quote of multiline block
    STATE_STRING_MULTILINE_END_3, // multiline string closed

    // declaration
    STATE_AT_SYMBOL, // @
    STATE_MAIN_IDENTIFIER,
    STATE_COLON, // :
    STATE_COMMA, // ,
    STATE_DOT, // .

    // numbers
    STATE_INTEGER,
    STATE_INTEGER_UNDERSCORE, // 1_2_3
    STATE_ZERO,
    STATE_DECIMAL_POINT, // . double
    STATE_DOUBLE_WHOLE_NUMBER, // double without decimal point
    STATE_DOUBLE_WHOLE_NUMBER_UNDERSCORE,
    STATE_DOUBLE_DECIMAL_NUMBER, // numbers with decimal point
    STATE_DOUBLE_DECIMAL_NUMBER_UNDERSCORE,
    STATE_DOUBLE_EXPONENT, // E,e
    STATE_DOUBLE_EXPONENT_SIGN, // +/- exponent number
    STATE_DOUBLE_EXPONENT_NUMBER, // value of exponent
    STATE_DOUBLE_EXPONENT_NUMBER_UNDERSCORE,
    STATE_DOUBLE_END_D, // D or d

    // comparison
    STATE_LESSER_GREATER,
    STATE_ASSIGNMENT,
    STATE_COMPARE,

    // brackets
    STATE_LEFT_BRACKET,
    STATE_RIGHT_BRACKET,
    STATE_UNIT,
    STATE_RIGHT_CURLY_BRACKET,

    // block comment
    STATE_LINE_FEED,
    STATE_INDENT,
    STATE_SLASH, // / is operator, or start of the comment followed by /
    STATE_COMMENT_START,
    STATE_BLOCK_COMMENT_START, // /*
    STATE_ANOTHER_BLOCK_COMMENT_START,
    STATE_ANOTHER_BLOCK_COMMENT_END,
    STATE_BLOCK_COMMENT_END_WITH_LF,
    STATE_BLOCK_COMMENT_END_WITHOUT_LF,

    // alphabet
    STATE_IDENTIFIER,
    STATE_UNDERSCORE_IDENTIFIER,

    // interpolates
    STATE_CHARACTER_S,
    STATE_STRING_INTERPOLATION_WHICH_1,
    STATE_STRING_INTERPOLATION_WHICH_2,
    STATE_MULTILINE_STRING_INTERPOLATION,
    STATE_MULTILINE_STRING_INTERPOLATION_ESCAPE,
    STATE_MULTILINE_STRING_INTERPOLATION_DOLLAR,
    STATE_MULTILINE_STRING_INTERPOLATION_EXPRESSION,
    STATE_MULTILINE_STRING_INTERPOLATION_END_1,
    STATE_MULTILINE_STRING_INTERPOLATION_END_2,
    STATE_MULTILINE_STRING_INTERPOLATION_END_3,
    STATE_STRING_INTERPOLATION,
    STATE_STRING_INTERPOLATION_DOLLAR,
    STATE_STRING_INTERPOLATION_EXPRESSION,
    STATE_STRING_INTERPOLATION_ESCAPE,
    STATE_STRING_INTERPOLATION_END,

} State;

typedef struct {
    char* chars;
    size_t len;
    size_t capacity;
} ScannedString;

typedef struct {
    bool beginning_of_file;
    bool inside_interpolated_string;
    ScannedString* scanned_string;
    char scanned_char_buffer;
    bool read_buffer; // will be set to true when scanner has no transition and the processed lexeme is valid
    bool lf_inside_block_comment;
    int block_comment_counter;
} ScannerContext;

/**
 * @brief FSM reads characters and identifies tokens.
 *
 * @param context Pointer to the scanner context (variables and pointers that the scanner needs to remember between individual tokens).
 * @param data Shared compiler data (variables and pointers used across multiple modules)
 * @param stream Input file stream to read from.
 * @return Token representing current lexeme.
 */
Token get_token(ScannerContext* context, SharedData* data, FILE* stream);

/**
 * @brief Initializes whole scanner_context.
 *
 * First time init only. If called over existing data, possible memory loss may occur.
 *
 * @param scanner_context Pointer to the scanner context.
 * @return 0 on success, 1 on memory allocation fail.
 */
int init_scanner(ScannerContext* scanner_context);

/** Destroys and frees the whole scanner_context.
 * @param scanner_context
 */
void destroy_scanner(const ScannerContext* scanner_context);

/**
 * @brief Allocates and initializes a dynamic string structure.
 *
 * @return Pointer to the allocated ScannedString, or NULL on malloc fail.
 */
ScannedString* init_scanned_string();

/**
 * @brief Doubles the capacity of the string buffer.
 *
 * @param string Pointer to the scanned string.
 * @return 0 on success, 1 on realloc fail.
 */
int resize_scanned_string(ScannedString* string);

/**
 * @brief Empties the dynamic string (without freeing memory).
 *
 * @param string Pointer to the scanned string.
 */
void clear_scanned_string(ScannedString* string);

/**
 * @brief Appends character to the string, resizing if necessary.
 * @param string Pointer to the scanned string.
 * @param scanned_char Character to append.
 * @return 0 on success, 1 on memory allocation fail.
 */
int scanned_string_append(ScannedString* string, char scanned_char);

/**
 * @brief Removes the last character from the string.
 *
 * @param string Pointer to the scanned string.
 */
void scanned_string_remove_last(ScannedString* string);

/**
 * @brief Returns the last character in the string.
 *
 * @param string Pointer to the scanned string.
 * @return The last character, or '\0' if the string is empty.
 */
char scanned_string_top(const ScannedString* string);

/**
 * @brief Frees the string structure and its buffer.
 *
 * @param string Pointer to the scanned string.
 */
void dispose_scanned_string(ScannedString* string);

/**
 * @brief Compares scanned string with the list of keywords for match.
 *
 * @param string Pointer to the scanned string.
 * @return -2 on NULL string pointer, IS_IDENTIFIER when the scanned string is not in the list of keywords, index of the keyword on match.
 */
int check_identifier_for_kw(ScannedString* string);

#endif // IFJ_1_COMPILER_SCANNER_H