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

// keywords
#define NUMBER_OF_KEYWORDS 24
#define IS_IDENTIFIER (-1)
#define IS_BOOLEAN_VALUE_TRUE 22
#define IS_BOOLEAN_VALUE_FALSE 23

// interpolated string types
#define NO_INTERPOLATION 0
#define SIMPLE_INTERPOLATED_STRING 1
#define MULTILINE_INTERPOLATED_STRING 2

// indentation types
#define INVALID_INDENTATION (-1)
#define NO_INDENTATION 0
#define INDENTATION 1
#define DEDENTATION 2

#include "error.h"
#include "shared_strings.h"
#include "stack.h"

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

enum type_enum {
    NONE,
    ERROR,
    INTEGER,
    DOUBLE,
    OPERATOR,
    IDENTIFIER,
    MAIN_IDENTIFIER,
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
    DEDENT,
    LINE_FEED,
    WHITESPACE,
    ASSIGNMENT,
    COMPARE,
    LEFT_BRACKET,
    RIGHT_BRACKET,
    END_OF_FILE,
    SEMICOLON,
};

typedef struct token {
    enum type_enum type;

    union {
        long attribute_int;
        double attribute_double;
        const char* attribute_string;
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
    STATE_SEMICOLON, // ;
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

    STATE_END_OF_FILE,
    // block comment
    STATE_LINE_FEED,
    STATE_INDENT,
    STATE_SLASH, // / is operator, or start of the comment followed by /
    STATE_COMMENT,
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

    STATE_TAB,
} State;

typedef struct indent_el {
    int value;
    struct indent_el* next;
} IndentEl;

typedef struct {
    IndentEl* top;
} IndentStack;

typedef struct {
    bool beginning_of_file;
    int interpolation;

    // we need to store master/parent type of string interpolation
    // for return to correct type after processing expression inside the interpolated string
    int master_interpolated_string;
    DynamicString* scanned_string;
    Stack* indent_stack;
    int pending_indent;
    int number_of_pending_dedents;
    char scanned_char_buffer;
    bool read_buffer; // will be set to true when scanner has no transition and the processed lexeme is valid
    bool lf_inside_block_comment;
    int block_comment_counter;
    bool scanning_variable_inside_string_interpolation;
    bool pending_eof;
    int line;
    bool pending_dot;

    AllocatedStrings* strings;
} ScannerContext;

/**
 * @brief FSM reads characters and identifies tokens.
 *
 * @param context Pointer to the scanner context (variables and pointers that the scanner needs to remember between individual tokens).
 * @param stream Input file stream to read from.
 * @return Token representing current lexeme.
 */
Token get_token(ScannerContext* context, FILE* stream);

/**
 * @brief Initializes whole scanner_context.
 *
 * First time init only. If called over existing data, possible memory loss may occur.
 *
 * @param scanner_context Pointer to the scanner context.
 * @return 0 on success, 1 on memory allocation fail.
 */
int init_scanner(ScannerContext* scanner_context, AllocatedStrings* strings);

/** Destroys and frees the whole scanner_context.
 * @param scanner_context Pointer to the scanner context.
 */
void destroy_scanner(const ScannerContext* scanner_context);

/**
 * @brief Compares scanned string with the list of keywords for match.
 *
 * @param string Pointer to the scanned string.
 * @return -2 on NULL string pointer, IS_IDENTIFIER when the scanned string is not in the list of keywords, index of the keyword on match.
 */
int check_identifier_for_kw(DynamicString* string);

/**
 * @brief Compares current indent value with values on stack. And returns corresponding macro.
 *
 * If current indent value is identical with the value on top of the stack AND
 *  it was scanner calling this function (first_call == true), NO_INDENTATION is returned.
 *
 * If current indent value is larger than the value on top of the stack AND
 *  it was scanner calling this function (first_call == true), INDENTATION is returned.
 *
 * If current indent value is smaller than the value on top of the stack,
 *  context's attribute number of pending dedents is increased by one and the stack is popped.
 *  Then the function recursively calls itself to either find matching indent and return DEDENTATION
 *  with number of dedents stored in the context OR to find an indent which is now larger then the current value
 *  meaning it's an INVALID_INDENTATION.
 *
 * Examples:
 *
 *  NO_INDENTATION
 *   stack: | 4 |
 *   current value: 4
 *
 *  INDENTATION
 *   stack: | 4 |
 *   current value: 8
 *
 *  DEDENTATION
 *   stack | 4 |
 *   current value: 0
 *
 *  INVALID_INDENTATION
 *   stack: | 4 |
 *   current value: 2
 *
 * @param context Pointer to scanner context containing indentation stack.
 * @param current_indent Indentation value used for the comparison and other actions.
 * @param first_call Whether the function is being called by the scanner or recursively by the function itself.
 * @return INDENTATION, DEDENTATION, NO_INDENTATION or INVALID_INDENTATION based on specification.
 */
int check_indentation(ScannerContext* context, int current_indent, bool first_call);

#endif // IFJ_1_COMPILER_SCANNER_H