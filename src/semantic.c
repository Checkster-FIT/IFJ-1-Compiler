/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    semantic.c
 * @brief   Runs semantic checks on the AST and fills semantic information
 *          (data types, conversions, term types, ...)
 * @author  Wyslych Vojtech (xwyslyv00)
 */

#include "semantic.h"
#include "ast.h"
#include "debug.h"
#include "error.h"

#include <string.h>

char* type_full_name(char c)
{
    switch (c) {
        case 'i':
            return "Int";
        case 'd':
            return "Double";
        case 's':
            return "String";
        case 'u':
            return "Unit";
        case 'b':
            return "Boolean";
        case 'a':
            return "Any";
        default:
            return "unknown";
    }
}

int handle_block(ASTNodeList list, char* result_type)
{
    int result = 0;
    *result_type = 'u';

    ASTNode* statement = list.first;

    while (statement) {
        result = handle_statement(statement, result_type, NULL);
        if (result)
            return result;

        statement = statement->next;
    }

    return result;
}

int handle_statement(ASTNode* statement_node, char* result_type, char* dot_funcall_type)
{
    int result = 0;
    SymtableEntry* entry;

    switch (statement_node->type) {
        case AST_IF: {
            char then_t = 'u';
            char else_t = 'u';
            result = handle_expression(statement_node->if_statement.condition, result_type);
            if (result)
                return result;

            if (*result_type != 'b')
                return print_error(SEMANTIC_IF_WHILE_CONDITION_NOT_BOOL, statement_node->line, "if");

            ASTNode* then_n = statement_node->if_statement.then_node;
            ASTNode* else_n = statement_node->if_statement.else_node;

            result = handle_block(then_n->then_statement.statements, result_type);
            if (result)
                return result;

            then_t = *result_type;

            if (else_n) {
                result = handle_block(else_n->else_statement.statements, result_type);
                if (result)
                    return result;

                else_t = *result_type;
            } else
                else_t = 'u';

            if (then_t == else_t && then_t == 'r')
                *result_type = 'r';

            else if (then_t == else_t && then_t == 'u')
                *result_type = 'u';

            else
                *result_type = 'e';

            return 0;
        }

        case AST_WHILE_DO:
            result = handle_expression(statement_node->while_do.condition, result_type);
            if (result)
                return result;

            if (*result_type != 'b')
                return print_error(SEMANTIC_IF_WHILE_CONDITION_NOT_BOOL, statement_node->line, "while");

            result = handle_block(statement_node->while_do.statements, result_type);
            if (result)
                return result;

            // while block can have return - its type is checked in AST_RETURN and here is just Unit as
            //  block results are not propagated and its return type is set as Unit
            *result_type = 'u';
            return 0;

        case AST_THEN:
            result = handle_block(statement_node->then_statement.statements, result_type);
            if (result)
                return result;

            return 0;

        case AST_ELSE:
            result = handle_block(statement_node->else_statement.statements, result_type);
            if (result)
                return result;

            return 0;

        case AST_VAR_DEF:
            entry = get_closest_symtable_entry(statement_node, statement_node->vardef.identifier);
            if (!entry)
                return print_error(SEMANTIC_USE_OF_UNDEFINED_VAL_FUN, statement_node->line, statement_node->vardef.identifier);
            result = handle_expression(statement_node->vardef.expression, result_type);
            if (result)
                return result;
            if (!entry->data_type)
                entry->data_type = *result_type;

            // e.x. `val x: Double = 3`
            if (entry->data_type == 'd' && *result_type == 'i') {
                ASTNode* new = ast_insert_conversion(statement_node->vardef.expression, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, statement_node->line, "conversion node");
                *result_type = 'd';
            }

            if (entry->data_type != *result_type)
                return print_error(SEMANTIC_NOT_MATCHING_VAR_TYPE, statement_node->line, type_full_name(entry->data_type), type_full_name(*result_type));

            if (entry->defined)
                return print_error(SEMANTIC_VARIABLE_OR_FUNCTION_REDEFINITION, statement_node->line);

            entry->defined = true;
            *result_type = 'u';
            return 0;

        case AST_VAR_ASSIGN:
            entry = get_closest_defined_symtable_entry(statement_node, statement_node->assignment.target);
            if (!entry)
                return print_error(SEMANTIC_USE_OF_UNDEFINED_VAL_FUN, statement_node->line, statement_node->assignment.target);
            if (entry->function || entry->constant)
                return print_error(SEMANTIC_WRITE_INTO_CONST_OR_FUN, statement_node->line, entry->identifier);

            result = handle_expression(statement_node->assignment.expression, result_type);
            if (result)
                return result;

            if (entry->data_type == 'd' && *result_type == 'i') {
                ASTNode* new = ast_insert_conversion(statement_node->assignment.expression, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, statement_node->line, "conversion node");
                *result_type = 'd';
            }

            if (entry->data_type != *result_type) {
                return print_error(SEMANTIC_NOT_MATCHING_VAR_TYPE, statement_node->line, type_full_name(entry->data_type), type_full_name(*result_type));
            }

            *result_type = 'u';
            return 0;

        case AST_RETURN: {
            // find node of function inside which the return is used
            ASTNode* node = statement_node->parent;
            while (node->type != AST_FUNCTION) // parser doesn't let return slip outside function
                node = node->parent;

            entry = get_closest_symtable_entry(node, node->function.identifier);
            if (!entry)
                return print_error(SEMANTIC_STANDALONE_RETURN, statement_node->line);

            ASTNode* expr = statement_node->return_st.expression;

            if (!expr) {
                if (entry->data_type != 'u')
                    return print_error(SEMANTIC_RETURN_NOT_ALLOWED, statement_node->line, entry->identifier, type_full_name(entry->data_type));

                *result_type = 'r';
                return 0;
            }

            result = handle_expression(statement_node->return_st.expression, result_type);
            if (result)
                return result;

            if (entry->data_type == 'u') {
                *result_type = 'r';
                return 0;
            }

            // e.x. `return 5` in f(): Double
            if (entry->data_type == 'd' && *result_type == 'i') {
                ASTNode* new = ast_insert_conversion(statement_node->return_st.expression, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, statement_node->line, "conversion node");
                *result_type = 'r';
                return 0;
            }

            if (!entry->data_type)
                return print_error(SEMANTIC_RETURN_IN_F_WITHOUT_RETURN_TYPE, statement_node->line, &entry->identifier[2]); // remove e.x. `i#` from the function identifier

            // compare return type with declared type
            if (entry->data_type != *result_type)
                return print_error(SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE, statement_node->line);

            *result_type = 'r';
            return 0;
        }

        case AST_LITERAL: {
            char t = statement_node->literal.type;
            switch (t) {
                case AST_INTEGER:
                    *result_type = 'i';
                    break;
                case AST_DOUBLE:
                    *result_type = 'd';
                    break;
                case AST_BOOLEAN:
                    *result_type = 'b';
                    break;
                case AST_STRING:
                    *result_type = 's';
                    break;
                case AST_UNIT:
                    *result_type = 'u';
                    break;
                default:
                    *result_type = 'u';
                    break;
            }

            return 0;
        }

        case AST_EXPRESSION:
            return handle_expression(statement_node, result_type);

        case AST_BUILTIN_FUNCALL:
        case AST_FUNCALL:
        case AST_VAR_USE:
        case AST_TERM: {
            DynamicString* identifier_tmp = init_dynamic_string();

            if (!identifier_tmp)
                return print_error(MEMORY_ALLOCATION_ERROR, statement_node->line, "dynamic string");

            if (dot_funcall_type) {
                if (strcmp(statement_node->term.identifier, "toString") == 0)
                    dynamic_string_append(identifier_tmp, 'a');
                else
                    dynamic_string_append(identifier_tmp, *dot_funcall_type);

                dynamic_string_append(identifier_tmp, '%');
                dynamic_string_append_str(identifier_tmp, statement_node->term.identifier);
            } else {
                dynamic_string_append_str(identifier_tmp, statement_node->term.identifier);
            }

            // Save the original funcall type
            const char tmp_type = dot_funcall_type ? *dot_funcall_type : '\0';

            // get closest either function or defined variable
            entry = get_closest_defined_symtable_entry(statement_node, identifier_tmp->chars);
            if (dot_funcall_type) {
                // if not found, try implicit conversion to Double
                if (!entry && *dot_funcall_type == 'i') {
                    clear_dynamic_string(identifier_tmp);
                    dynamic_string_append_str(identifier_tmp, "d%");
                    dynamic_string_append_str(identifier_tmp, statement_node->term.identifier);
                    *dot_funcall_type = 'c'; // conversion
                } else
                    *dot_funcall_type = 'n'; // no conversion
            }

            entry = get_closest_defined_symtable_entry(statement_node, identifier_tmp->chars);

            if (!entry) {
                int exit_code = 0;

                if (!dot_funcall_type)
                    exit_code = print_error(SEMANTIC_USE_OF_UNDEFINED_VAL_FUN, statement_node->line, identifier_tmp->chars);
                else
                    exit_code = print_error(SEMANTIC_EXTENSION_INVALID_CALL_TYPE, statement_node->line,
                        statement_node->term.identifier, str_type(tmp_type));

                dispose_dynamic_string(identifier_tmp);
                return exit_code;
            }

            dispose_dynamic_string(identifier_tmp);

            if (entry->function) {

                // funcall and/or recursion
                if (!entry->data_type) {
                    if (entry->status == ANALYSIS_IN_PROGRESS)
                        return print_error(SEMANTIC_RECURSION_WITHOUT_TYPE_DEF, statement_node->line, entry->identifier);

                    // reach program
                    ASTNode* node = statement_node->parent;
                    while (node->type != AST_PROGRAM) // parser doesn't let return slip outside function
                        node = node->parent;

                    node = find_function(node, entry->identifier);

                    result = handle_function(node);
                    if (result)
                        return result;

                    if (!entry->data_type)
                        // handle function was not able to determine the return type of this function
                        return print_error(SEMANTIC_UNABLE_TO_DETERMINE_RETURN_TYPE, node->line, entry->identifier);
                }

                char* param_string = entry->parameters->chars;
                int i = 0; // marking char read from the parameter string

                // ----- PARAM CHECKS -----

                for (ASTNode* group = statement_node->term.argument_groups.first; group; group = group->next) {
                    if (param_string[i] != '#') {
                        if (entry->extension)
                            return print_error(SEMANTIC_FUNCALL_PARAMETER_MISMATCH, statement_node->line, &entry->identifier[2]);
                        return print_error(SEMANTIC_FUNCALL_PARAMETER_MISMATCH, statement_node->line, entry->identifier);
                    }
                    i++;

                    for (ASTNode* arg = group->argument_group.arguments.first; arg; arg = arg->next) {
                        if (param_string[i] == '#' || param_string[i] == '\0') { // called with too many args
                            if (entry->extension)
                                return print_error(SEMANTIC_FUNCALL_PARAMETER_MISMATCH, statement_node->line, &entry->identifier[2]);
                            return print_error(SEMANTIC_FUNCALL_PARAMETER_MISMATCH, statement_node->line, entry->identifier);
                        }

                        result = handle_expression(arg, result_type);
                        if (result)
                            return result;

                        if (*result_type == 'i' && param_string[i] == 'd') {
                            ASTNode* new = ast_insert_conversion(arg, 'd');
                            if (!new)
                                return print_error(MEMORY_ALLOCATION_ERROR, statement_node->line, "conversion node");
                            *result_type = 'd';
                        }

                        // check if the parameter is not `Any` type
                        if (*result_type != param_string[i] && param_string[i] != 'a')
                            return print_error(SEMANTIC_FUNCALL_PARAMETER_TYPE_MISMATCH, statement_node->line);

                        i++;
                    }
                }

                if (param_string[i] != '\0') { // called with too little args
                    if (entry->extension)
                        return print_error(SEMANTIC_FUNCALL_PARAMETER_MISMATCH, statement_node->line, &entry->identifier[2]);
                    return print_error(SEMANTIC_FUNCALL_PARAMETER_MISMATCH, statement_node->line, entry->identifier);
                }

                if (entry->builtin)
                    statement_node->type = AST_BUILTIN_FUNCALL;
                else
                    statement_node->type = AST_FUNCALL;

                statement_node->function.identifier = entry->identifier;
                *result_type = entry->data_type;
                return 0;
            }

            ASTNode* par = statement_node->term.argument_groups.first;
            if (par)
                return print_error(SEMANTIC_ARGUMENT_PASSED_TO_VARIABLE, statement_node->line, entry->identifier);

            *result_type = entry->data_type;
            statement_node->type = AST_VAR_USE;
            return 0;
        }

        case AST_INTERPOLATED_STRING:
            result = handle_block(statement_node->interpolated_string.expressions, result_type);
            if (result)
                return result;

            *result_type = 's';
            return 0;

        case AST_CONVERSION:
            *result_type = statement_node->conversion.result_type;
            return 0;

        default:
            break;
    }

    return result;
}

ASTNode* find_function(ASTNode* node, const char* identifier)
{
    ASTNode* f;
    ASTNode* ext;
    // search for desired function in extensions
    for (ext = node->program.extensions.first; ext; ext = ext->next) {
        for (f = ext->extension.functions.first; f; f = f->next) {
            if (strcmp(f->function.identifier, identifier) == 0)
                return f;
        }
    }

    // search for desired function in normal functions
    for (f = node->program.functions.first; f; f = f->next) {
        if (strcmp(f->function.identifier, identifier) == 0)
            return f;
    }

    return NULL;
}

int handle_expression(ASTNode* node, char* result_type)
{
    int result = 0;
    char right_result_type;
    char* op = "";

    if (node->type != AST_EXPRESSION)
        return handle_statement(node, result_type, NULL);

    int oper = node->expression.operation;

    switch (oper) {
        case OP_NOT:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;

            if (*result_type != 'b')
                return print_error(SEMANTIC_NEGATION_OF_NON_BOOL_TYPE, node->line, type_full_name(*result_type));

            node->expression.result_type = *result_type;

            return 0;

        case OP_UNARY_PLUS:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;

            if (*result_type != 'i' && *result_type != 'd')
                return print_error(SEMANTIC_UNARY_OPERATION_NOT_OVER_NUMBER, node->line, "+", type_full_name(*result_type));

            node->expression.result_type = *result_type;

            return 0;

        case OP_UNARY_MINUS:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;

            if (*result_type != 'i' && *result_type != 'd')
                return print_error(SEMANTIC_UNARY_OPERATION_NOT_OVER_NUMBER, node->line, "-", type_full_name(*result_type));

            node->expression.result_type = *result_type;

            return 0;

        case OP_MUL:
        case OP_DIV:
        case OP_ADD:
        case OP_SUB:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;
            result = handle_expression(node->expression.right, &right_result_type);
            if (result)
                return result;

            if (*result_type == 'd' && right_result_type == 'd') {
                node->expression.result_type = 'd';
                return 0;
            }

            if (*result_type == 'i' && right_result_type == 'i') {
                node->expression.result_type = 'i';
                return 0;
            }

            if (*result_type == 'i' && right_result_type == 'd') {
                ASTNode* new = ast_insert_conversion(node->expression.left, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'd';
                node->expression.result_type = 'd';
                return 0;
            }

            if (*result_type == 'd' && right_result_type == 'i') {
                ASTNode* new = ast_insert_conversion(node->expression.right, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'd';
                node->expression.result_type = 'd';
                return 0;
            }

            if (oper == OP_ADD && *result_type == 's' && right_result_type == 's') {
                node->expression.result_type = 's';
                return 0;
            }

            op = "";
            switch (oper) {
                case OP_MUL:
                    op = "*";
                    break;
                case OP_DIV:
                    op = "/";
                    break;
                case OP_SUB:
                    op = "-";
                    break;
                case OP_ADD:
                    op = "+";
                    return print_error(SEMANTIC_NOT_OVER_NUMBERS_OR_STRINGS, node->line, op, type_full_name(*result_type), type_full_name(right_result_type));
                default:
                    break;
            }

            return print_error(SEMANTIC_NUMERIC_OPERATION_NOT_OVER_NUMBERS, node->line, op, type_full_name(*result_type), type_full_name(right_result_type));

        case OP_LESS:
        case OP_LESS_EQUAL:
        case OP_GREATER:
        case OP_GREATER_EQUAL:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;
            result = handle_expression(node->expression.right, &right_result_type);
            if (result)
                return result;

            if (*result_type == 'd' && right_result_type == 'd') {
                *result_type = 'b';
                node->expression.result_type = 'b';
                return 0;
            }

            if (*result_type == 'i' && right_result_type == 'i') {
                *result_type = 'b';
                node->expression.result_type = 'b';
                return 0;
            }

            if (*result_type == 'i' && right_result_type == 'd') {
                ASTNode* new = ast_insert_conversion(node->expression.left, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'b';
                node->expression.result_type = 'b';
                return 0;
            }

            if (*result_type == 'd' && right_result_type == 'i') {
                ASTNode* new = ast_insert_conversion(node->expression.right, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'b';
                node->expression.result_type = 'b';
                return 0;
            }

            if (*result_type == 's' && right_result_type == 's') {
                *result_type = 'b';
                node->expression.result_type = 'b';
                return 0;
            }

            op = "";
            switch (oper) {
                case OP_LESS:
                    op = "<";
                    break;
                case OP_LESS_EQUAL:
                    op = "<=";
                    break;
                case OP_GREATER:
                    op = ">";
                    break;
                case OP_GREATER_EQUAL:
                    op = ">=";
                    break;
                default:
                    break;
            }

            return print_error(SEMANTIC_NOT_OVER_NUMBERS_OR_STRINGS, node->line, op, type_full_name(*result_type), type_full_name(right_result_type));

        case OP_EQUAL:
        case OP_NOT_EQUAL:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;
            result = handle_expression(node->expression.right, &right_result_type);
            if (result)
                return result;

            if (*result_type == right_result_type) {
                *result_type = 'b';
                node->expression.result_type = 'b';
                return 0;
            }

            if (*result_type == 'i' && right_result_type == 'd') {
                ASTNode* new = ast_insert_conversion(node->expression.left, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'b';
                node->expression.result_type = 'b';
                return 0;
            }

            if (*result_type == 'd' && right_result_type == 'i') {
                ASTNode* new = ast_insert_conversion(node->expression.right, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
                *result_type = 'b';
                node->expression.result_type = 'b';
                return 0;
            }

            op = "";
            switch (oper) {
                case OP_EQUAL:
                    op = "==";
                    break;
                case OP_NOT_EQUAL:
                    op = "!=";
                    break;
                default:
                    break;
            }

            return print_error(SEMANTIC_EQUAL_NOT_EQUAL_TYPE_MISMATCH, node->line, op, type_full_name(*result_type), type_full_name(right_result_type));

        case OP_AND:
        case OP_OR:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;
            result = handle_expression(node->expression.right, &right_result_type);
            if (result)
                return result;

            if (*result_type != 'b' || right_result_type != 'b') {
                if (oper == OP_AND)
                    return print_error(SEMANTIC_BINARY_OP_NOT_BOOL, node->line, "&&", type_full_name(*result_type), type_full_name(right_result_type));

                return print_error(SEMANTIC_BINARY_OP_NOT_BOOL, node->line, "||", type_full_name(*result_type), type_full_name(right_result_type));
            }

            node->expression.result_type = *result_type;

            return 0;

        case OP_DOT:
            result = handle_expression(node->expression.left, result_type);
            if (result)
                return result;

            // handle_statement() checks if suitable for dot notation (matching types)
            result = handle_statement(node->expression.right, &right_result_type, result_type); // hand expression on which type the valmem or ext function has been called
            if (result)
                return result;

            if (*result_type == 'c') {
                ASTNode* new = ast_insert_conversion(node->expression.left, 'd');
                if (!new)
                    return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
            }

            *result_type = right_result_type;
            node->expression.result_type = right_result_type;

            return 0;

        default:
            break;
    }

    return result;
}

int handle_function(ASTNode* node)
{
    int result = 0;
    char result_type = 'u';

    SymtableEntry* entry = get_closest_symtable_entry(node, node->function.identifier);

    // using == finished would cause infinite recursive checks
    // in other words = only analyse function of which the analysis has not started yet
    if (entry->status != ANALYSIS_NONE)
        return 0;

    entry->status = ANALYSIS_IN_PROGRESS;

    result = handle_block(node->function.statements, &result_type);
    if (result)
        return result;

    // when else and if types don't match, 'e' is propagated, but it's valid for Unit function
    if (result_type == 'e' && entry->data_type != 'u')
        return print_error(SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE, node->line);

    if (!entry->data_type) {
        entry->data_type = result_type;
    }

    // e.x. `def f(): Double = \n 3+3`
    if (result_type == 'i' && entry->data_type == 'd') {
        ASTNode* new = ast_insert_conversion(node->function.statements.last, 'd');
        if (!new)
            return print_error(MEMORY_ALLOCATION_ERROR, node->line, "conversion node");
        result_type = 'd';
    }

    // --- type mismatch check ---
    // e.x. f(): Int = \n return ()
    // Unit function performs semantic analysis and result_type is ignored
    // return data type is already checked in AST_RETURN, so only 'r' is propagated here
    if (result_type != entry->data_type && result_type != 'r' && entry->data_type != 'u')
        return print_error(SEMANTIC_NOT_MATCHING_FUNCTION_RETURN_DATA_TYPE, node->line);

    entry->status = ANALYSIS_FINISHED;

    return 0;
}

int main_check(ASTNode* node, bool* main_found)
{
    SymtableEntry* entry = get_closest_symtable_entry(node, node->function.identifier);

    if (*main_found && entry->main)
        return print_error(SEMANTIC_MULTIPLE_MAIN, node->line);

    if (entry->main) {
        if (entry->parameters->len > 1) // "#", "" = no args
            return print_error(SEMANTIC_MAIN_WITH_PARAMETERS, node->line);

        // main_check() is called after the function has been analysed
        // if the return type is not set to Unit in the symtable, it's not determinable
        if (entry->data_type != 'u')
            return print_error(SEMANTIC_MAIN_NOT_UNIT, node->line);

        *main_found = true;
    }

    return 0;
}

// TODO: kontrola rekurze u ext
int run_semantic(ASTNode* ast)
{
    int result = 0;
    ASTNode* fun_node = NULL;
    bool main_found = false;

    if (ast->type != AST_PROGRAM) {
        return print_error(CORRUPTED_MEMORY, 0);
    }

    ASTNode* ext_node = ast->program.extensions.first;

    // cycle through extensions
    while (ext_node) {

        fun_node = ext_node->extension.functions.first;

        // cycle through ext functions
        while (fun_node) {
            result = handle_function(fun_node);
            if (result)
                return result;

            // parser checks for @main in ext functions
            fun_node = fun_node->next;
        }
        ext_node = ext_node->next;
    }

    fun_node = ast->program.functions.first;

    // cycle through normal functions
    while (fun_node) {
        result = handle_function(fun_node);
        if (result)
            return result;

        result = main_check(fun_node, &main_found);
        if (result)
            return result;

        fun_node = fun_node->next;
    }

    if (!main_found)
        return print_error(SEMANTIC_NO_MAIN, 0);

    return 0;
}