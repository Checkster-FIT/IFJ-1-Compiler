/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    scanner.h
 * @brief   Declaration of the Token type, get_token() and enumeration of the FSM states used to implement the scanner.
 * @author  Drahovska Andrea, Wyslych Vojtech
 */

#ifndef IFJ_1_COMPILER_SCANNER_H
#define IFJ_1_COMPILER_SCANNER_H
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    bool beginning_of_file;
    bool inside_interpolated_string;
} ScannerContext;

typedef struct {
    enum {

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
        char* attribute_str_ptr;
        void* attribute_none;
    };
} Token;

typedef enum {
    STATE_START,

    // operators
    STATE_AMPERSAND_1, // &
    STATE_OR_1, // |
    STATE_OPERATOR, // +, -, *, &&, ||

    // strings
    STATE_STRING_WHICH, // " read
    STATE_STRING_BEGIN,
    STATE_STRING_ESCAPE, // \ read
    STATE_STRING_END,
    STATE_EMPTY_STRING, // "", can be empty string

    // multiline string """
    STATE_STRING_MULTILINE_START, // """ start of multiline string
    STATE_STRING_MULTILINE_END_1,
    STATE_STRING_MULTILINE_END_2,
    STATE_STRING_MULTILINE_END_3,

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
    STATE_STRING_INTERPOLATION_END

} State;

// TODO: Add descriptions
Token get_token(ScannerContext context, FILE* stream);
int init_scanner(ScannerContext* scanner_context);

#endif // IFJ_1_COMPILER_SCANNER_H