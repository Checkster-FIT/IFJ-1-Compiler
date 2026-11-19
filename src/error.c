/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    error.c
 * @brief   Contains error handling logic used across modules
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "error.h"
#include "parser.h"
#include "scanner.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

ErrorMessage get_error_by_id(const enum error_enum error_id)
{
    ErrorMessage error;
    error.id = error_id;
    error.fix = "";
    error.param_count = 0;

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
            error.param_count = 2;
            error.code = 1;
            break;
        case SCANNER_NUM_INVALID_UNDERSCORE:
            error.message = "Underscore(s) inside an integer can only be used between two numbers: '%s' is incorrect";
            error.fix = "Remove underscore(s) or put numbers around them";
            error.param_count = 1;
            error.code = 1;
            break;
        case SCANNER_DOUBLE_INVALID_DECIMAL_POINT:
            error.message = "Decimal point must be followed by a number: '%s'";
            error.fix = "Add a number right after the decimal point";
            error.param_count = 1;
            error.code = 1;
            break;
        case SCANNER_DOUBLE_MISSING_EXPONENT:
            error.message = "Missing exponent of Double: '%s'; e/E (may be followed by a sign) must be followed by a number";
            error.fix = "Remove the e/E or add an exponent";
            error.param_count = 1;
            error.code = 1;
            break;
        case SCANNER_UNKNOWN_OPERATOR:
            error.message = "An unknown operator '%s' was found";
            error.fix = "Did you mean to use '%s'?";
            error.param_count = 2;
            error.code = 1;
            break;
        case SCANNER_INVALID_STRING:
            error.message = "A string cannot contain '%s': '%s'";
            error.fix = "End the string properly first";
            error.param_count = 2;
            error.code = 1;
            break;
        case SCANNER_INVALID_BLOCK_COMMENT:
            error.message = "A block comment needs to be ended before end of the file";
            error.param_count = 0;
            error.code = 1;
            break;
        case SCANNER_INVALID_IDENTIFIER:
            error.message = "'%s' is not a valid identifier (only letters, digits, and underscores are allowed)";
            error.fix = "Identifier cannot begin with a digit. An underscore is not a valid identifier by itself";
            error.param_count = 1;
            error.code = 1;
            break;
        case SCANNER_BLOCK_COMMENT_FOLLOWED_BY_CHARS:
            error.message = "A block comment cannot be followed by non-whitespace characters on the same line: '%s'";
            error.fix = "Remove the characters between the block comment and end of line";
            error.param_count = 1;
            error.code = 1;
            break;
        case SCANNER_INVALID_INDENTATION:
            error.message = "Indentation used in an incorrect way";
            error.fix = "Align indentation correctly";
            error.param_count = 0;
            error.code = 1;
            break;
        case SCANNER_INVALID_INDENTATION_USING_TAB:
            error.message = "Tab cannot be used to indentation";
            error.fix = "Replace tab with spaces";
            error.param_count = 0;
            error.code = 1;
            break;
        // Syntax errors
        case SYNTAX_INCORRECT_USAGE_OF_AT_SYMBOL:
            error.message = "The '@' symbol must be directly followed by the identifier 'main'";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_SYNTAX_ERROR:
            error.message = "Expected %s but got %s instead";
            error.param_count = 2;
            error.code = 2;
            break;
        case PARSER_GLOBAL_SCOPE_INVALID:
            error.message = "Invalid use of '%s' in the global scope";
            error.param_count = 1;
            error.code = 2;
            break;
        case PARSER_MISSING_IMPORT:
            error.message = "Missing import header";
            error.fix = "Add the required 'import ifj26.*' header at the start of the file";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_MISSING_HEADER:
            error.message = "Missing or incorrect 'import ifj26.*' header";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_MISSING_EXT_PARAM:
            error.message = "Missing extension parameter";
            error.fix = "The 'extension' keyword must be followed by a valid parameter: 'extension (param: Type)'";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_MISSING_EXT_PARAM_END:
            error.message = "Extension allows only one parameter";
            error.fix = "Add ')' after the first extension parameter: 'extension (param: Type)'";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_EXT_MISSING_NEWLINE:
            error.message = "An extension needs to be followed by a newline";
            error.fix = "Add a line break after the extension before the function definitions";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_EXT_MISSING_INDENT:
            error.message = "The extension block needs to be indented";
            error.fix = "Add an indent in front of any function definitions inside the extension";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_EXT_BLOCK_INVALID:
            error.message = "The extension block can only contain extension functions";
            error.fix = "Did you mean to end the extension instead?";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_INVALID_FUNCTION:
            error.message = "Missing required function identifier";
            error.fix = "The 'def' keyword must be followed by a valid identifier";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_MISSING_PARAM_BRACKET:
            error.message = "Missing closing ) bracket after function parameters";
            error.fix = "Add ) at the end of the function parameter list";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_MISSING_PARAM_TYPE:
            error.message = "Missing parameter type specifier";
            error.fix = "Make sure every parameter has its type correctly specified '(param : Type)'";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_INCORRECT_TYPE:
            error.message = "Unknown variable type: '%s'";
            error.fix = "Valid parameter types include: (String, Int, Double, Unit, Boolean)";
            error.param_count = 1;
            error.code = 2;
            break;
        case PARSER_INVALID_PARAM_LIST_END:
            error.message = "Missing comma separator or closing bracket after function parameter list";
            error.fix = "Add ',' between function parameters or ')' to close the parameter list: '(param1 : Int, param2: Int)'";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_INVALID_PARAM:
            error.message = "Expected a parameter declaration or closing bracket";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_DOUBLE_PARAM_TYPE:
            error.message = "Multiple data types specified for a single function parameter";
            error.fix = "Valid parameter format: 'param: Type'";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_MISSING_FUNCTION_ASSIGNMENT:
            error.message = "Expected '=' after function declaration";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_FUNCTION_MISSING_INDENT:
            error.message = "A multiline function declaration block needs to be indented";
            error.fix = "Add a same size indent in front of all statements inside the function";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_FUNCTION_MISSING_BODY:
            error.message = "Function definition must be followed by an indented block or an expression";
            error.fix = "Empty block or expression can be represented by the unit literal: '()'";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_ST_VAR_DEF_MISSING_IDENTIFIER:
            error.message = "Missing variable identifier in its definition";
            error.fix = "Add a valid identifier name after the variable definition keyword (var, val)";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_ST_VAR_DEF_MISSING_ASSIGNMENT:
            error.message = "Expected '=' after variable declaration";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_ST_MISSING_SEPARATOR:
            error.message = "A statement must be ended with a new line or a semicolon ';'";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_FUNCTION_SINGLE_LINE_MISSING_LINE_FEED:
            error.message = "A single-line function declaration must be ended with a new line";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_WHILE_MISSING_DO:
            error.message = "Expected 'do' keyword after while loop condition";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_WHILE_MISSING_LINE_FEED:
            error.message = "A while loop block must start on a new line";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_WHILE_MISSING_INDENT:
            error.message = "A while loop block needs to be indented";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_IF_MISSING_THEN:
            error.message = "Expected 'then' keyword after if statement condition";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_IF_MISSING_INDENT:
            error.message = "A multiline if statement needs to be indented";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_INVALID_EXPRESSION_USE:
            error.message = "Invalid use of '%s' inside an expression";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_INVALID_EXPRESSION_FORMAT:
            error.message = "Invalid expression format";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_INVALID_FUNCTION_DEF:
            error.message = "Expected 'def' keyword after main function annotation";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_MULTIPLE_IMPORT_HEADERS:
            error.message = "Multiple import headers are not allowed";
            error.param_count = 0;
            error.code = 2;
            break;
        case PARSER_MISSING_PARAM_IDENTIFIER:
            error.message = "Missing parameter identifier before type specifier ':'";
            error.param_count = 0;
            error.code = 2;
            break;
        // Semantic error
        case SEMANTIC_INVALID_REDEFINITION:
            error.message = "Multiple definitions found for '%s'";
            error.param_count = 0;
            error.code = 4;
            break;
        case SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE:
            error.message = "Value returned by the function does not match its specified return type";
            error.param_count = 0;
            error.code = 6;
            break;
        case SEMANTIC_MULTIPLE_MAIN:
            error.message = "Two or more functions are marked with `@main`";
            error.fix = "Exactly one function must be marked as the entry function";
            error.param_count = 0;
            error.code = 4;
            break;
        case SEMANTIC_NO_MAIN:
            error.message = "No function is marked with `@main`";
            error.fix = "Exactly one function must be marked as the entry function";
            error.param_count = 0;
            error.code = 3;
            break;
        case SEMANTIC_VARIABLE_OR_FUNCTION_REDEFINITION:
            error.message = "Repeated definition of function, variable, or value in the same block is not allowed";
            error.fix = "Remove repeated definition and use assignment";
            error.param_count = 0;
            error.code = 4;
            break;
        case SEMANTIC_NEGATION_OF_NON_BOOL_TYPE:
            error.message = "Unable to perform unary negation on type '%s'";
            error.fix = "Negation can only be performed on the 'Boolean' data type";
            error.param_count = 1;
            error.code = 6;
            break;
        case SEMANTIC_UNARY_OPERATION_NOT_OVER_NUMBER:
            error.message = "Unable to perform unary '%s' operator on type '%s'";
            error.fix = "Operand must be of type 'Int' or 'Double'";
            error.param_count = 2;
            error.code = 6;
            break;
        case SEMANTIC_NUMERIC_OPERATION_NOT_OVER_NUMBERS:
            error.message = "Unable to perform '%s' operation on type '%s' and '%s'";
            error.fix = "Both operands must be of type 'Int' or 'Double'";
            error.param_count = 3;
            error.code = 6;
            break;
        case SEMANTIC_NOT_OVER_NUMBERS_OR_STRINGS:
            error.message = "Unable to perform '%s' operation on type '%s' and '%s'";
            error.fix = "Operands must be combination of 'Int' and 'Double', or two 'String' operands";
            error.param_count = 3;
            error.code = 6;
            break;
        case SEMANTIC_EQUAL_NOT_EQUAL_TYPE_MISMATCH:
            error.message = "Unable to perform '%s' operation on type '%s' and '%s'";
            error.fix = "Operands must be 'Int' or 'Double' or two identical data types";
            error.param_count = 3;
            error.code = 6;
            break;
        case SEMANTIC_BINARY_OP_NOT_BOOL:
            error.message = "Unable to perform '%s' operation on type '%s' and '%s'";
            error.fix = "Both operands must be of type 'Boolean'";
            error.param_count = 3;
            error.code = 6;
            break;
        case SEMANTIC_IF_WHILE_CONDITION_NOT_BOOL:
            error.message = "Result of '%s' statement condition must be 'Boolean'";
            error.param_count = 1;
            error.code = 6;
            break;
        case SEMANTIC_NOT_MATCHING_VAR_TYPE:
            error.message = "Unable to assign '%s' value to variable of type '%s'",
            error.param_count = 2;
            error.code = 6;
            break;
        case SEMANTIC_USE_OF_UNDEFINED_VAL_FUN:
            error.message = "Cannot resolve '%s'";
            error.fix = "Function or variable must be defined before use";
            error.param_count = 1;
            error.code = 3;
            break;
        case SEMANTIC_EXTENSION_INVALID_CALL_TYPE:
            error.message = "Cannot resolve function call '%s' on the '%s' type";
            error.fix = "Function or variable must be defined before use with this type";
            error.param_count = 2;
            error.code = 6;
            break;
        case SEMANTIC_FUNCALL_PARAMETER_MISMATCH:
            error.message = "'%s' function call does not match the defined number of parameters or group of parameters";
            error.param_count = 2;
            error.code = 5;
            break;
        case SEMANTIC_FUNCALL_PARAMETER_TYPE_MISMATCH:
            error.message = "Type mismatch on function call parameter types";
            error.param_count = 0;
            error.code = 5;
            break;
        case SEMANTIC_ARGUMENT_PASSED_TO_VARIABLE:
            error.message = "Variables take no arguments - cannot pass arguments to '%s'";
            error.param_count = 1;
            error.code = 10;
            break;
        case SEMANTIC_STANDALONE_RETURN:
            error.message = "Standalone return was found";
            error.fix = "Return can only be placed inside a function";
            error.param_count = 0;
            error.code = 10;
            break;
        case SEMANTIC_RETURN_NOT_ALLOWED:
            error.message = "Unable to use 'return' without an expression in function '%s' with '%s' return type";
            error.fix = "Functions with return statements without an expression must have explicit 'Unit' return type";
            error.param_count = 2;
            error.code = 10;
            break;
        case SEMANTIC_RETURN_IN_F_WITHOUT_RETURN_TYPE:
            error.message = "Unable to use return in function '%s' as it has no explicit return type";
            error.fix = "Return can only be used inside function with explicit return type";
            error.param_count = 1;
            error.code = 10;
            break;
        case SEMANTIC_MAIN_WITH_PARAMETERS:
            error.message = "Function with the '@main' tag cannot accept any parameters";
            error.param_count = 0;
            error.code = 5;
            break;
        case SEMANTIC_MAIN_NOT_UNIT:
            error.message = "Return type of function with the '@main' tag must be 'Unit'";
            error.param_count = 0;
            error.code = 6;
            break;
        case SEMANTIC_WRITE_INTO_CONST_OR_FUN:
            error.message = "Cannot assign value to a constant variable, function parameter or a function: '%s'";
            error.param_count = 1;
            error.code = 7;
            break;
        case SEMANTIC_RECURSION_WITHOUT_TYPE_DEF:
            error.message = "Cannot perform recursive call of function '%s' as it has no declared return type";
            error.fix = "Add return type to function declaration";
            error.param_count = 1;
            error.code = 6;
            break;
        case SEMANTIC_DOT_NOTATION_TYPE_MISMATCH:
            error.message = "Unable to perform dot notation on mismatching types. Left side type: '%s', right side type: '%s'";
            error.param_count = 3;
            error.code = 6;
            break;
        case SEMANTIC_UNABLE_TO_DETERMINE_RETURN_TYPE:
            error.message = "Unable to determine return type of function '%s'";
            error.fix = "Add return type to function definition";
            error.param_count = 1;
            error.code = 6;
            break;
        // Internal errors
        case MEMORY_ALLOCATION_ERROR:
            error.message = "An error has occurred when trying to allocate memory for '%s'";
            error.param_count = 1;
            error.code = 99;
            break;
        case STREAM_READ_ERROR:
            error.message = "An error has occurred when trying to read from the input stream";
            error.param_count = 0;
            error.code = 99;
            break;
        case CORRUPTED_MEMORY:
            error.message = "An error has occurred when trying to read from memory in state: '%i'";
            error.param_count = 1;
            error.code = 99;
            break;
        // Program errors
        case PARSER_RULE_INCOMPLETE:
            error.message = "The required parser rule for '%s' is incomplete";
            error.param_count = 1;
            error.code = 100;
            break;
        case SYMTABLE_ENTRY_NOT_FOUND:
            error.message = "The identifier '%s' was not found in the given symtable";
            error.param_count = 1;
            error.code = 100;
            break;
        case PARSER_RULE_INCORRECT_CALL:
            error.message = "A rule was called when it does not match the next token";
            error.param_count = 0;
            error.code = 100;
            break;
        case SCANNER_NO_TRANSITION:
            error.message = "No scanner transition was found for state '%i' and character '%c'";
            error.fix = "Try adding this case in scanner.c";
            error.param_count = 2;
            error.code = 100;
            break;
        default:
            error.message = "An unknown error has occurred...?";
            error.fix = "Try adding a valid error message in the shared module";
            error.param_count = 0;
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

int print_error(const int error_id, const int line, ...)
{
    va_list args = { };
    va_start(args, line);

    const int code = va_print_error(error_id, line, args);

    va_end(args);
    return code;
}

int va_print_error(const int error_id, const int line, va_list args)
{
    // Gets the full error message for the error with this id
    const ErrorMessage error = get_error_by_id(error_id);

    // Build the error message string format from its components
    char error_message[1024];

    // Create the format with or without the fix
    const char* format = "\033[1;31m[%s] on line %i: \n"
                         "     | %s\n"
                         "     | %s\033[0m\n";
    if (strlen(error.fix) <= 0)
        format = "\033[1;31m[%s] on line %i: \n"
                 "     | %s\033[0m\n";

    snprintf(
        error_message,
        sizeof(error_message),
        format,
        get_error_type_str(error.code), line,
        error.message, error.fix);

    vfprintf(stderr, error_message, args);

    return error.code;
}