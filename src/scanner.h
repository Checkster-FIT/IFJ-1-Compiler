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

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    enum type_enum {
        NONE,
        ERROR,
        INTEGER,
        DOUBLE,
        OPERATOR,
        IDENTIFIER,
        KEYWORD,
        BOOLEAN,
        CHAR_S,
        EMPTY_STRING,
        STRING_INTERPOLATION,
        STRING_INTERPOLATION_END,
        STRING,
        MULTILINE_STRING,
        DOT,
        ZERO,
        DOUBLE_DECIMAL_NUMBER,
        DECLARATION_SIGN,
        COMMA,
        INDENT,
        LINE_FEED,
        WHITESPACE,
        GREATER_LESSER_THAN,
        ASSIGNMENT,
        COMPARE,
        LEFT_BRACKET,
        RIGHT_BRACKET,
        UNIT,
        RIGHT_CURLY_BRACKET,
        END_OF_FILE
    } type;

    union {
        int attribute_int;
        double attribute_double;
        const char* attribute_str_ptr;
        bool attribute_bool;
        void* attribute_none;

        enum {
            INTERNAL_ERROR,
            LEXICAL_ERROR,
        } attribute_error_type;

        enum {
            PLUS, // +
            MINUS, // -
            MULTIPLY, // *
            AND, // &&
            OR, // ||
            NOT, // !
            DIVIDE, // /
        } attribute_operation_type;

        enum {
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
        } attribute_keyword;
    };
} Token;

typedef enum {

    STATE_START, // initial state dispatcher

    // operators
    STATE_AMPERSAND_1, // expecting second '&' for AND operator
    STATE_OR_1, // expecting second '|' for OR operator
    STATE_OPERATOR, // operator complete, resolve type:  +, -, *, &&, ||

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
    STATE_KEYWORD,
    STATE_BOOLEAN,

    // interpolates
    STATE_CHARACTER_S,
    STATE_STRING_INTERPOLATION,
    STATE_STRING_INTERPOLATION_DOLLAR,
    STATE_STRING_INTERPOLATION_EXPRESSION,
    STATE_STRING_INTERPOLATION_ESCAPE,
    STATE_STRING_INTERPOLATION_END,

    STATE_END_OF_FILE,

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
} ScannerContext;

// TODO: Add descriptions
Token get_token(ScannerContext* context, FILE* stream);

/** Initializes whole scanner_context.
 * First time init only. If called over existing data, possible memory loss may occur.
 *
 * @param scanner_context
 * @return 0 on success, 1 on memory allocation fail
 */
int init_scanner(ScannerContext* scanner_context);

/**
 *
 * @return 0 on success, 1 on malloc fail
 */
ScannedString* init_scanned_string();

/**
 *
 * @param string
 * @return 0 on success, 1 on realloc fail
 */
int resize_scanned_string(ScannedString* string);

void clear_scanned_string(ScannedString* string);

/**
 *
 * @param string
 * @param scanned_char
 * @return 0 on success, 1 on memory allocation fail
 */
int scanned_string_append(ScannedString* string, char scanned_char);

void scanned_string_remove_last(ScannedString* string);

char scanned_string_top(const ScannedString* string);

void dispose_scanned_string(ScannedString* string);

#endif // IFJ_1_COMPILER_SCANNER_H