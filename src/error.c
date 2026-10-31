/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    error.c
 * @brief   Contains error handling logic used across modules
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "error.h"
#include "scanner.h"
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
        // Lexical error
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
        case SCANNER_INVALID_INDENTATION:
            error.message = "Indentation used in an incorrect way";
            error.fix = "Align indentation correctly";
            error.code = 1;
            break;
        case SCANNER_INVALID_INDENTATION_USING_TAB:
            error.message = "Tab cannot be used to indentation";
            error.fix = "Replace tab with space";
            error.code = 1;
            break;
        // Syntax errors
        case SYNTAX_INCORRECT_USAGE_OF_AT_SYMBOL:
            error.message = "The symbol @ can only be followed by identifier 'main'";
            error.code = 2;
            break;
        case PARSER_SYNTAX_ERROR:
            error.message = "Expected %s but got %s instead";
            error.code = 2;
            break;
        case PARSER_GLOBAL_SCOPE_INVALID:
            error.message = "Invalid use of %s in the global scope";
            error.code = 2;
            break;
        case PARSER_MISSING_IMPORT:
            error.message = "Missing import header";
            error.fix = "Add the required 'import ifj26.*' header at the start of the file";
            error.code = 2;
            break;
        case PARSER_MISSING_HEADER:
            error.message = "Missing or incorrect 'ifj26.*' header";
            error.code = 2;
            break;
        case PARSER_MISSING_EXT_PARAM:
            error.message = "Missing extension parameter";
            error.fix = "The 'extension' keyword should be followed by a valid parameter: 'extension (param: Type)'";
            error.code = 2;
            break;
        case PARSER_MISSING_EXT_PARAM_END:
            error.message = "Extension allows only one parameter";
            error.fix = "Add ')' after the first extension parameter: 'extension (param: Type)'";
            error.code = 2;
            break;
        case PARSER_EXT_MISSING_NEWLINE:
            error.message = "An extension needs to be followed by a newline";
            error.fix = "Add a line break after the extension before the function definitions";
            error.code = 2;
            break;
        case PARSER_EXT_MISSING_INDENT:
            error.message = "The extension block needs to be indented";
            error.fix = "Add an indent in front of any function definitions inside the extension";
            error.code = 2;
            break;
        case PARSER_EXT_BLOCK_INVALID:
            error.message = "The extension block can only contain extension functions";
            error.fix = "Did you mean to end the extension instead?";
            error.code = 2;
            break;
        case PARSER_INVALID_FUNCTION:
            error.message = "Missing function identifier";
            error.fix = "The 'def' keyword should be followed by a valid identifier";
            error.code = 2;
            break;
        case PARSER_MISSING_PARAM_BRACKET:
            error.message = "Missing closing ) bracket after function parameters";
            error.fix = "Add ) at the end of the function parameter list";
            error.code = 2;
            break;
        case PARSER_MISSING_PARAM_TYPE:
            error.message = "Missing parameter type specifier";
            error.fix = "Make sure every parameter has its type correctly specified '(param : Type)'";
            error.code = 2;
            break;
        case PARSER_INCORRECT_TYPE:
            error.message = "Unknown type";
            error.fix = "Valid parameter types include: (String, Int, Double, Unit, Boolean)";
            error.code = 2;
            break;
        case PARSER_INVALID_PARAM_LIST_END:
            error.message = "Missing comma separator or closing bracket after function parameter list";
            error.fix = "Add ',' between function parameters or ')' to close the parameter list: '(param1 : Int, param2: Int)'";
            error.code = 2;
            break;
        case PARSER_INVALID_PARAM:
            error.message = "Expected a parameter declaration or closing bracket";
            error.code = 2;
            break;
        case PARSER_DOUBLE_PARAM_TYPE:
            error.message = "Extra parameter type specification";
            error.fix = "Remove the second type specification";
            error.code = 2;
            break;
        case PARSER_MISSING_FUNCTION_ASSIGNMENT:
            error.message = "Expected '=' after function declaration";
            error.code = 2;
            break;
        case PARSER_FUNCTION_MISSING_INDENT:
            error.message = "A multiline function declaration block needs to be indented";
            error.fix = "Add a same size indent in front of all statements inside the function";
            error.code = 2;
            break;
        case PARSER_ST_VAR_DEF_MISSING_IDENTIFIER:
            error.message = "Missing variable identifier in its definition";
            error.fix = "Add a valid identifier name after the variable definition keyword (var, val)";
            error.code = 2;
            break;
        case PARSER_ST_VAR_DEF_MISSING_ASSIGNMENT:
            error.message = "Expected '=' after variable declaration";
            error.code = 2;
            break;
        case PARSER_ST_MISSING_LINE_FEED:
            error.message = "A statement must be ended with a new line";
            error.code = 2;
            break;
        case PARSER_FUNCTION_SINGLE_LINE_MISSING_LINE_FEED:
            error.message = "A single-line function declaration must be ended with a new line";
            error.code = 2;
            break;
        // Internal errors
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
        // Program errors
        case PARSER_INCOMPLETE_RULE:
            error.message = "The required parser rule for '%s' is incomplete";
            // error.fix = "Try adding this rule in parser.c";
            error.code = 100;
            break;
        case SCANNER_NO_TRANSITION:
            error.message = "No scanner transition was found for state '%i' and character '%c'";
            error.fix = "Try adding this case in scanner.c";
            error.code = 100;
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

const char* get_token_type_str(const int type)
{
    switch (type) {
        case NONE:
            return "NONE";
        case ERROR:
            return "ERROR";
        case INTEGER:
            return "INTEGER";
        case DOUBLE:
            return "DOUBLE";
        case OPERATOR:
            return "OPERATOR";
        case IDENTIFIER:
            return "IDENTIFIER";
        case KEYWORD:
            return "KEYWORD";
        case BOOLEAN:
            return "BOOLEAN";
        case STRING_INTERPOLATION:
            return "STRING_INTERPOLATION";
        case STRING_INTERPOLATION_END:
            return "STRING_INTERPOLATION_END";
        case MULTILINE_STRING_INTERPOLATION:
            return "MULTILINE_STRING_INTERPOLATION";
        case MULTILINE_STRING_INTERPOLATION_END:
            return "MULTILINE_STRING_INTERPOLATION_END";
        case STRING:
            return "STRING";
        case MULTILINE_STRING:
            return "MULTILINE_STRING";
        case DOT:
            return "DOT";
        case DECLARATION_SIGN:
            return "DECLARATION_SIGN";
        case COMMA:
            return "COMMA";
        case INDENT:
            return "INDENT";
        case LINE_FEED:
            return "LINE_FEED";
        case WHITESPACE:
            return "WHITESPACE";
        case ASSIGNMENT:
            return "ASSIGNMENT";
        case COMPARE:
            return "COMPARE";
        case LEFT_BRACKET:
            return "LEFT_BRACKET";
        case RIGHT_BRACKET:
            return "RIGHT_BRACKET";
        case END_OF_FILE:
            return "END_OF_FILE";
        default:
            return "UNKNOWN_TOKEN";
    }
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
        format = "\033[1;31m[%s]: %s\033[0m\n";

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