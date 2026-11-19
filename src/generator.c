/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator.c
 * @brief   Generates '.ifj26code' from the final AST and the included symtables
 * @author  Tobias Mikan (xmikant00)
 */

#include "generator.h"
#include "ast.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int builtin_label_id = 0;

void generate_program(ASTNode* node)
{
    printf(".IFJcode26\n\n");
    if (node == NULL || node->type != AST_PROGRAM) {
        fprintf(stderr, "generate_program: expected AST_PROGRAM\n");
        exit(99);
    }

    SymtableEntry* main_entry = symtable_main_search(node->program.global_symtable);
    if (main_entry == NULL) {
        fprintf(stderr, "generate_program: entry function not found\n");
        exit(99);
    }

    printf("JUMP %s\n\n", main_entry->identifier);

    ASTNode* helper_node = node->program.functions.first;
    while (helper_node != NULL) {
        generate_function(helper_node);
        helper_node = helper_node->next;
    }

    ASTNode* helper_ext_node = node->program.extensions.first;
    while (helper_ext_node != NULL) {
        ASTNode* helper_func_node = helper_ext_node->extension.functions.first;
        while (helper_func_node != NULL) {
            generate_function(helper_func_node);
            helper_func_node = helper_func_node->next;
        }
        helper_ext_node = helper_ext_node->next;
    }
    printf("EXIT int@0\n");
}

static ASTNode* current_function_node = NULL;

void generate_function(ASTNode* node)
{
    if (node->parent == NULL) {
        return;
    }

    printf("LABEL %s\n", node->function.identifier);

    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
    printf("DEFVAR LF@$discard\n");
    printf("DEFVAR LF@$op1\n");
    printf("DEFVAR LF@$op2\n");
    printf("DEFVAR LF@$op3\n");
    printf("DEFVAR LF@$tmp_type\n");
    printf("DEFVAR LF@$tmp_char\n");
    generate_local_variables(node->function.symtable);
    if (node->parent->type == AST_EXTENSION) {
        generate_local_variables(node->parent->extension.symtable);
    }
    generate_nested_local_variables(node->function.statements.first);
    current_function_node = node;
    generate_parameters(&node->function.parameter_groups);
    if (node->parent->type == AST_EXTENSION) {
        generate_parameters(&node->parent->extension.parameter_groups);
    }

    bool is_main = (node->parent->type == AST_PROGRAM) && strcmp(symtable_main_search(node->parent->program.global_symtable)->identifier, node->function.identifier) == 0;

    ASTNode* last = node->function.statements.last;
    bool has_expr_ret = !is_main && last && (last->type == AST_EXPRESSION || last->type == AST_FUNCALL || last->type == AST_BUILTIN_FUNCALL || last->type == AST_VAR_USE || last->type == AST_INTERPOLATED_STRING);

    generate_statements(node->function.statements.first, has_expr_ret);

    if (is_main) {
        printf("EXIT int@0\n\n");
    } else {
        if (!has_expr_ret)
            printf("PUSHS nil@nil\n");
        printf("POPFRAME\nRETURN\n\n");
    }
}

void generate_nested_local_variables(ASTNode* node)
{
    while (node != NULL) {
        if (node->type == AST_IF) {
            if (node->if_statement.then_node != NULL) {
                generate_local_variables(node->if_statement.then_node->then_statement.symtable);
                ASTNode* then_statement = node->if_statement.then_node->then_statement.statements.first;
                generate_nested_local_variables(then_statement);
            }
            if (node->if_statement.else_node != NULL) {
                generate_local_variables(node->if_statement.else_node->else_statement.symtable);
                ASTNode* else_statement = node->if_statement.else_node->else_statement.statements.first;
                generate_nested_local_variables(else_statement);
            }
        }
        if (node->type == AST_WHILE_DO) {
            generate_local_variables(node->while_do.symtable);
            ASTNode* while_do_statement = node->while_do.statements.first;
            generate_nested_local_variables(while_do_statement);
        }
        node = node->next;
    }
}

void generate_local_variables_recursive(TNode* node)
{
    if (node == NULL) {
        return;
    }
    generate_local_variables_recursive(node->left);
    SymtableEntry* entry = (SymtableEntry*)node->value;
    printf("DEFVAR LF@%s_%zx\n", entry->identifier, (uintptr_t)entry);
    entry->defined = false;
    generate_local_variables_recursive(node->right);
}

void generate_local_variables(Symtable* symtable)
{
    if (symtable == NULL) {
        return;
    }
    if (symtable->root == NULL) {
        return;
    }
    generate_local_variables_recursive(symtable->root);
}

void generate_parameter(ASTNode* param_node)
{
    if (param_node == NULL) {
        return;
    }

    generate_parameter(param_node->next);

    if (param_node->parameter.identifier != NULL) {
        SymtableEntry* entry = get_closest_symtable_entry(param_node, param_node->parameter.identifier);
        if (entry == NULL) {
            entry = get_closest_symtable_entry(current_function_node, param_node->parameter.identifier);
        }
        if (entry == NULL) {
            fprintf(stderr, "generate_parameter: parameter '%s' not found in symtable\n", param_node->parameter.identifier);
            exit(99);
        }
        entry->defined = true;
        printf("POPS LF@%s_%zx\n", param_node->parameter.identifier, (uintptr_t)entry);
    }
}

void generate_parameter_groups(ASTNode* group_node)
{
    if (group_node == NULL) {
        return;
    }
    generate_parameter_groups(group_node->next);
    generate_parameter(group_node->parameter_group.parameters.first);
}

void generate_parameters(ASTNodeList* param_group_list)
{
    if (param_group_list == NULL) {
        return;
    }
    generate_parameter_groups(param_group_list->first);
}

void generate_arguments(ASTNodeList* arguments)
{
    if (arguments == NULL) {
        return;
    }
    ASTNode* argument_group = arguments->first;
    while (argument_group != NULL) {
        ASTNode* argument = argument_group->argument_group.arguments.first;
        while (argument != NULL) {
            generate_expression(argument);
            argument = argument->next;
        }
        argument_group = argument_group->next;
    }
}

void generate_builtin_function(ASTNode* node)
{
    if (strcmp(node->term.identifier, "toStr") == 0 || strcmp(node->term.identifier, "a%toString") == 0) {
        ASTNode* builtin_group = node->term.argument_groups.first;
        while (builtin_group != NULL) {
            ASTNode* arg = builtin_group->argument_group.arguments.first;
            while (arg != NULL) {
                generate_expression(arg);
                arg = arg->next;
            }
            builtin_group = builtin_group->next;
        }
        to_str();
    } else if (strcmp(node->term.identifier, "strLength") == 0 || strcmp(node->term.identifier, "s%length") == 0) {
        ASTNode* builtin_group = node->term.argument_groups.first;
        while (builtin_group != NULL) {
            ASTNode* arg = builtin_group->argument_group.arguments.first;
            while (arg != NULL) {
                generate_expression(arg);
                arg = arg->next;
            }
            builtin_group = builtin_group->next;
        }
        printf("POPS LF@$discard\n");
        printf("STRLEN LF@$discard LF@$discard\n");
        printf("PUSHS LF@$discard\n");
    } else if (strcmp(node->term.identifier, "d%toInt") == 0) {
        printf("FLOAT2INTS\n");
    } else if (strcmp(node->term.identifier, "d%floor") == 0) {
        int id_builtin = builtin_label_id++;
        printf("POPS LF@$op1\n");
        printf("FLOAT2INT LF@$op2 LF@$op1\n");
        printf("INT2FLOAT LF@$op3 LF@$op2\n");
        printf("GT LF@$tmp_type LF@$op3 LF@$op1\n");
        printf("JUMPIFEQ builtin_end_%d LF@$tmp_type bool@false\n", id_builtin);
        printf("SUB LF@$op3 LF@$op3 float@0x1p+0\n");
        printf("LABEL builtin_end_%d\n", id_builtin);
        printf("PUSHS LF@$op3\n");
    }
}

void generate_expression(ASTNode* node)
{
    if (node == NULL) {
        return;
    }
    switch (node->type) {
        case AST_LITERAL: {
            char* type;
            switch (node->literal.type) {
                case AST_INTEGER:
                    type = "int";
                    printf("PUSHS %s@%d\n", type, node->literal.val_integer);
                    break;
                case AST_DOUBLE:
                    type = "float";
                    printf("PUSHS %s@%a\n", type, node->literal.val_double);
                    break;
                case AST_BOOLEAN:
                    type = "bool";
                    char* value = node->literal.val_bool ? "true" : "false";
                    printf("PUSHS %s@%s\n", type, value);
                    break;
                case AST_STRING:
                    type = "string";
                    int i = 0;
                    unsigned char* string = (unsigned char*)node->literal.val_string;
                    printf("PUSHS %s@", type);
                    while (string[i] != '\0') {
                        if (string[i] <= 32 || string[i] == 35 || string[i] == 92) {
                            printf("\\%03d", string[i]);
                        } else {
                            printf("%c", string[i]);
                        }
                        i++;
                    }
                    printf("\n");
                    break;
                case AST_UNIT:
                    printf("PUSHS nil@nil\n");
                    break;
                case AST_UNKNOWN_LITERAL:
                    fprintf(stderr, "generate_expression: unknown literal\n");
                    exit(99);
            }
            break;
        }
        case AST_TERM:
        case AST_VAR_USE:
            SymtableEntry* entry = get_closest_defined_symtable_entry(node, node->term.identifier);
            printf("PUSHS LF@%s_%zx\n", node->term.identifier, (uintptr_t)entry);
            break;
        case AST_EXPRESSION:
            if (node->expression.operation == OP_AND) {
                int id_and = builtin_label_id++;
                generate_expression(node->expression.left);
                to_bool();
                printf("PUSHS bool@false\n");
                printf("JUMPIFEQS and_false_%d\n", id_and);
                generate_expression(node->expression.right);
                to_bool();
                printf("JUMP and_end_%d\n", id_and);
                printf("LABEL and_false_%d\n", id_and);
                printf("PUSHS bool@false\n");
                printf("LABEL and_end_%d\n", id_and);
            } else if (node->expression.operation == OP_OR) {
                int id_or = builtin_label_id++;
                generate_expression(node->expression.left);
                to_bool();
                printf("PUSHS bool@true\n");
                printf("JUMPIFEQS or_true_%d\n", id_or);
                generate_expression(node->expression.right);
                to_bool();
                printf("JUMP or_end_%d\n", id_or);
                printf("LABEL or_true_%d\n", id_or);
                printf("PUSHS bool@true\n");
                printf("LABEL or_end_%d\n", id_or);
            } else {
                switch (node->expression.operation) {
                    case OP_MUL:
                        generate_expression(node->expression.left);
                        generate_expression(node->expression.right);
                        printf("MULS\n");
                        break;
                    case OP_DIV:
                        generate_expression(node->expression.left);
                        generate_expression(node->expression.right);
                        int id_div = builtin_label_id++;
                        printf("POPS LF@$op2\n");
                        printf("POPS LF@$op1\n");
                        printf("TYPE LF@$tmp_type LF@$op1\n");
                        printf("JUMPIFEQ $div_int_%d LF@$tmp_type string@int\n", id_div);
                        printf("DIV LF@$op1 LF@$op1 LF@$op2\n");
                        printf("JUMP $div_end_%d\n", id_div);
                        printf("LABEL $div_int_%d\n", id_div);
                        printf("IDIV LF@$op1 LF@$op1 LF@$op2\n");
                        printf("LABEL $div_end_%d\n", id_div);
                        printf("PUSHS LF@$op1\n");
                        break;
                    case OP_ADD:
                        generate_expression(node->expression.left);
                        generate_expression(node->expression.right);
                        if (is_string_expr(node)) {
                            printf("POPS LF@$op2\n");
                            printf("POPS LF@$op1\n");
                            printf("CONCAT LF@$op1 LF@$op1 LF@$op2\n");
                            printf("PUSHS LF@$op1\n");
                        } else {
                            printf("ADDS\n");
                        }
                        break;
                    case OP_SUB:
                        generate_expression(node->expression.left);
                        generate_expression(node->expression.right);
                        printf("SUBS\n");
                        break;
                    case OP_NOT:
                        generate_expression(node->expression.left);
                        to_bool();
                        printf("NOTS\n");
                        break;
                    case OP_UNARY_PLUS:
                        break;
                    case OP_UNARY_MINUS:
                        generate_expression(node->expression.left);
                        int id_unary_minus = builtin_label_id++;
                        printf("POPS LF@$op1\n");
                        printf("TYPE LF@$tmp_type LF@$op1\n");
                        printf("JUMPIFEQ unary_minus_float_%d LF@$tmp_type string@float\n", id_unary_minus);
                        printf("JUMPIFEQ unary_minus_int_%d LF@$tmp_type string@int\n", id_unary_minus);
                        printf("LABEL unary_minus_int_%d\n", id_unary_minus);
                        printf("SUB LF@$op1 int@0 LF@$op1\n");
                        printf("JUMP unary_minus_end_%d\n", id_unary_minus);
                        printf("LABEL unary_minus_float_%d\n", id_unary_minus);
                        printf("SUB LF@$op1 float@0x0p+0 LF@$op1\n");
                        printf("LABEL unary_minus_end_%d\n", id_unary_minus);
                        printf("PUSHS LF@$op1\n");
                        break;
                    case OP_LESS:
                        generate_expression(node->expression.left);
                        generate_expression(node->expression.right);
                        printf("LTS\n");
                        break;
                    case OP_GREATER:
                        generate_expression(node->expression.left);
                        generate_expression(node->expression.right);
                        printf("GTS\n");
                        break;
                    case OP_EQUAL:
                        generate_expression(node->expression.left);
                        generate_expression(node->expression.right);
                        printf("EQS\n");
                        break;
                    case OP_LESS_EQUAL:
                        generate_expression(node->expression.left);
                        generate_expression(node->expression.right);
                        printf("GTS\n");
                        printf("NOTS\n");
                        break;
                    case OP_GREATER_EQUAL:
                        generate_expression(node->expression.left);
                        generate_expression(node->expression.right);
                        printf("LTS\n");
                        printf("NOTS\n");
                        break;
                    case OP_NOT_EQUAL:
                        generate_expression(node->expression.left);
                        generate_expression(node->expression.right);
                        printf("EQS\n");
                        printf("NOTS\n");
                        break;
                    case OP_DOT:
                        generate_expression(node->expression.left);
                        generate_arguments(&node->expression.right->term.argument_groups);
                        if (node->expression.right->type == AST_BUILTIN_FUNCALL) {
                            generate_builtin_function(node->expression.right);
                        } else {
                            printf("CREATEFRAME\n");
                            printf("CALL %s\n", node->expression.right->term.identifier);
                        }
                        break;
                    case OP_NONE:
                        fprintf(stderr, "generate_expression: operator isn't set\n");
                        exit(99);
                    case OP_TERMINAL:
                        fprintf(stderr, "generate_expression: operator type is terminal\n");
                        exit(99);
                    default:
                        fprintf(stderr, "generate_expression: accessed wrong operand type\n");
                        exit(99);
                }
            }
            break;
        case AST_INTERPOLATED_STRING:
            printf("PUSHS string@\n");
            ASTNode* expr = node->interpolated_string.expressions.first;
            while (expr != NULL) {
                generate_expression(expr);
                to_str();
                expr = expr->next;
                printf("POPS LF@$op2\n");
                printf("POPS LF@$op1\n");
                printf("CONCAT LF@$op1 LF@$op1 LF@$op2\n");
                printf("PUSHS LF@$op1\n");
            }
            break;
        case AST_FUNCTION:
            break;
        case AST_FUNCALL:
            generate_arguments(&node->term.argument_groups);
            printf("CREATEFRAME\n");
            printf("CALL %s\n", node->term.identifier);
            break;
        case AST_BUILTIN_FUNCALL: {
            if (node->term.identifier != NULL) {
                if (strcmp(node->term.identifier, "print") == 0) {
                    ASTNode* builtin_group = node->term.argument_groups.first;
                    while (builtin_group != NULL) {
                        ASTNode* arg = builtin_group->argument_group.arguments.first;
                        while (arg != NULL) {
                            generate_expression(arg);
                            int id_print = builtin_label_id++;
                            printf("POPS LF@$discard\n");
                            printf("JUMPIFEQ write_unit_%d LF@$discard nil@nil\n", id_print);
                            printf("WRITE LF@$discard\n");
                            printf("JUMP write_end_%d\n", id_print);
                            printf("LABEL write_unit_%d\n", id_print);
                            printf("WRITE string@()\n");
                            printf("LABEL write_end_%d\n", id_print);
                            arg = arg->next;
                        }
                        builtin_group = builtin_group->next;
                    }
                    printf("PUSHS nil@nil\n");
                } else if (strcmp(node->term.identifier, "readInt") == 0) {
                    builtin_label_id++;
                    printf("READ LF@$discard int\n");
                    printf("TYPE LF@$tmp_type LF@$discard\n");
                    printf("JUMPIFEQ read_is_nil_%d LF@$tmp_type string@nil\n", builtin_label_id);
                    printf("PUSHS LF@$discard\n");
                    printf("JUMP read_is_end_%d\n", builtin_label_id);
                    printf("LABEL read_is_nil_%d\n", builtin_label_id);
                    printf("PUSHS int@0\n");
                    printf("LABEL read_is_end_%d\n", builtin_label_id);
                } else if (strcmp(node->term.identifier, "readIntMaybe") == 0) {
                    printf("READ LF@$discard int\n");
                    printf("PUSHS LF@$discard\n");
                } else if (strcmp(node->term.identifier, "readDouble") == 0) {
                    builtin_label_id++;
                    printf("READ LF@$discard float\n");
                    printf("TYPE LF@$tmp_type LF@$discard\n");
                    printf("JUMPIFEQ read_is_nil_%d LF@$tmp_type string@nil\n", builtin_label_id);
                    printf("PUSHS LF@$discard\n");
                    printf("JUMP read_is_end_%d\n", builtin_label_id);
                    printf("LABEL read_is_nil_%d\n", builtin_label_id);
                    printf("PUSHS float@0x0p+0\n");
                    printf("LABEL read_is_end_%d\n", builtin_label_id);
                } else if (strcmp(node->term.identifier, "readDoubleMaybe") == 0) {
                    printf("READ LF@$discard float\n");
                    printf("PUSHS LF@$discard\n");
                } else if (strcmp(node->term.identifier, "readLine") == 0) {
                    printf("READ LF@$discard string\n");
                    printf("PUSHS LF@$discard\n");
                } else if (strcmp(node->term.identifier, "readBoolean") == 0) {
                    int id_bool = builtin_label_id++;
                    printf("READ LF@$discard string\n");
                    printf("TYPE LF@$tmp_type LF@$discard\n");
                    printf("JUMPIFEQ read_bool_false_%d LF@$tmp_type string@nil\n", id_bool);
                    printf("JUMPIFEQ read_bool_true_%d LF@$discard string@true\n", id_bool);
                    printf("JUMPIFEQ read_bool_true_%d LF@$discard string@true\\010\n", id_bool);
                    printf("JUMPIFEQ read_bool_true_%d LF@$discard string@true\\013\\010\n", id_bool);
                    printf("LABEL read_bool_false_%d\n", id_bool);
                    printf("PUSHS bool@false\n");
                    printf("JUMP read_bool_end_%d\n", id_bool);
                    printf("LABEL read_bool_true_%d\n", id_bool);
                    printf("PUSHS bool@true\n");
                    printf("LABEL read_bool_end_%d\n", id_bool);
                } else if (strcmp(node->term.identifier, "strLength") == 0) {
                    ASTNode* builtin_group = node->term.argument_groups.first;
                    while (builtin_group != NULL) {
                        ASTNode* arg = builtin_group->argument_group.arguments.first;
                        while (arg != NULL) {
                            generate_expression(arg);
                            arg = arg->next;
                        }
                        builtin_group = builtin_group->next;
                    }
                    printf("POPS LF@$discard\n");
                    printf("STRLEN LF@$discard LF@$discard\n");
                    printf("PUSHS LF@$discard\n");
                } else if (strcmp(node->term.identifier, "chr") == 0) {
                    ASTNode* builtin_group = node->term.argument_groups.first;
                    builtin_label_id++;
                    while (builtin_group != NULL) {
                        ASTNode* arg = builtin_group->argument_group.arguments.first;
                        while (arg != NULL) {
                            generate_expression(arg);
                            arg = arg->next;
                        }
                        builtin_group = builtin_group->next;
                        printf("POPS LF@$op1\n");
                        printf("LT LF@$tmp_type LF@$op1 int@0\n");
                        printf("JUMPIFEQ chr_fail_%d LF@$tmp_type bool@true\n", builtin_label_id);
                        printf("GT LF@$tmp_type LF@$op1 int@127\n");
                        printf("JUMPIFEQ chr_fail_%d LF@$tmp_type bool@true\n", builtin_label_id);
                        printf("PUSHS LF@$op1\n");
                        printf("INT2CHARS\n");
                        printf("JUMP chr_success_%d\n", builtin_label_id);
                        printf("LABEL chr_fail_%d\n", builtin_label_id);
                        printf("INT2CHAR LF@$op1 int@-1\n");
                        printf("LABEL chr_success_%d\n", builtin_label_id);
                        builtin_label_id++;
                    }
                } else if (strcmp(node->term.identifier, "toInt") == 0) {
                    ASTNode* builtin_group = node->term.argument_groups.first;
                    while (builtin_group != NULL) {
                        ASTNode* arg = builtin_group->argument_group.arguments.first;
                        while (arg != NULL) {
                            generate_expression(arg);
                            arg = arg->next;
                        }
                        builtin_group = builtin_group->next;
                    }
                    printf("FLOAT2INTS\n");

                } else if (strcmp(node->term.identifier, "toDouble") == 0) {
                    ASTNode* builtin_group = node->term.argument_groups.first;
                    builtin_label_id++;
                    while (builtin_group != NULL) {
                        ASTNode* arg = builtin_group->argument_group.arguments.first;
                        while (arg != NULL) {
                            generate_expression(arg);
                            arg = arg->next;
                        }
                        builtin_group = builtin_group->next;
                    }
                    printf("INT2FLOATS\n");
                } else if (strcmp(node->term.identifier, "toStr") == 0) {
                    ASTNode* builtin_group = node->term.argument_groups.first;
                    while (builtin_group != NULL) {
                        ASTNode* arg = builtin_group->argument_group.arguments.first;
                        while (arg != NULL) {
                            generate_expression(arg);
                            arg = arg->next;
                        }
                        builtin_group = builtin_group->next;
                    }
                    to_str();
                } else if (strcmp(node->term.identifier, "ord") == 0) {
                    int id = builtin_label_id++;
                    ASTNode* builtin_group = node->term.argument_groups.first;
                    while (builtin_group != NULL) {
                        ASTNode* arg = builtin_group->argument_group.arguments.first;
                        while (arg != NULL) {
                            generate_expression(arg);
                            arg = arg->next;
                        }
                        builtin_group = builtin_group->next;
                    }
                    printf("POPS LF@$op2\n");
                    printf("POPS LF@$op1\n");
                    printf("STRLEN LF@$op3 LF@$op1\n");

                    printf("PUSHS LF@$op2\n");
                    printf("PUSHS int@0\n");
                    printf("LTS\n");
                    printf("PUSHS bool@true\n");
                    printf("JUMPIFEQS $ord_err_%d\n", id);
                    printf("PUSHS LF@$op2\n");
                    printf("PUSHS LF@$op3\n");
                    printf("LTS\n");
                    printf("PUSHS bool@false\n");
                    printf("JUMPIFEQS $ord_err_%d\n", id);
                    printf("STRI2INT LF@$discard LF@$op1 LF@$op2\n");
                    printf("PUSHS LF@$discard\n");
                    printf("JUMP $ord_end_%d\n", id);
                    printf("LABEL $ord_err_%d\n", id);
                    printf("PUSHS int@-1\n");
                    printf("LABEL $ord_end_%d\n", id);

                } else if (strcmp(node->term.identifier, "subStr") == 0) {
                    int id = builtin_label_id++;
                    ASTNode* builtin_group = node->term.argument_groups.first;
                    while (builtin_group != NULL) {
                        ASTNode* arg = builtin_group->argument_group.arguments.first;
                        while (arg != NULL) {
                            generate_expression(arg);
                            arg = arg->next;
                        }
                        builtin_group = builtin_group->next;
                    }
                    printf("POPS LF@$op3\n");
                    printf("POPS LF@$op2\n");
                    printf("POPS LF@$op1\n");
                    printf("MOVE LF@$discard string@\n");
                    printf("STRLEN LF@$tmp_type LF@$op1\n");

                    printf("PUSHS LF@$op2\n");
                    printf("PUSHS int@0\n");
                    printf("LTS\n");
                    printf("PUSHS bool@true\n");
                    printf("JUMPIFEQS $substr_err_%d\n", id);

                    printf("PUSHS LF@$op2\n");
                    printf("PUSHS LF@$op3\n");
                    printf("GTS\n");
                    printf("PUSHS bool@true\n");
                    printf("JUMPIFEQS $substr_err_%d\n", id);

                    printf("PUSHS LF@$op2\n");
                    printf("PUSHS LF@$tmp_type\n");
                    printf("LTS\n");
                    printf("PUSHS bool@false\n");
                    printf("JUMPIFEQS $substr_err_%d\n", id);

                    printf("PUSHS LF@$op3\n");
                    printf("PUSHS LF@$tmp_type\n");
                    printf("GTS\n");
                    printf("PUSHS bool@true\n");
                    printf("JUMPIFEQS $substr_err_%d\n", id);

                    printf("SUB LF@$op3 LF@$op3 LF@$op2\n");

                    printf("LABEL $substr_loop_%d\n", id);
                    printf("JUMPIFEQ $substr_end_%d LF@$op3 int@0\n", id);
                    printf("GETCHAR LF@$tmp_char LF@$op1 LF@$op2\n");
                    printf("CONCAT LF@$discard LF@$discard LF@$tmp_char\n");
                    printf("ADD LF@$op2 LF@$op2 int@1\n");
                    printf("SUB LF@$op3 LF@$op3 int@1\n");
                    printf("JUMP $substr_loop_%d\n", id);

                    printf("LABEL $substr_end_%d\n", id);
                    printf("PUSHS LF@$discard\n");
                    printf("JUMP $substr_ok_%d\n", id);

                    printf("LABEL $substr_err_%d\n", id);
                    printf("EXIT int@40\n");
                    printf("LABEL $substr_ok_%d\n", id);
                }
            }
            break;
        }
        case AST_CONVERSION:
            generate_expression(node->conversion.expression);
            switch (node->conversion.result_type) {
                case 'i':
                    printf("FLOAT2INTS\n");
                    break;
                case 'd':
                    printf("INT2FLOATS\n");
                    break;
                default:
                    fprintf(stderr, "generate_expression: unsupported conversion target type %d\n", node->conversion.result_type);
                    exit(99);
            }
            break;
        default:
            fprintf(stderr, "generate_expression: accessed wrong node type(%d)\n", node->type);
            exit(99);
    }
}

void generate_statements_switch(ASTNode* statement, bool is_last)
{
    switch (statement->type) {
        case AST_VAR_ASSIGN:
            generate_expression(statement->assignment.expression);
            SymtableEntry* entry = get_closest_defined_symtable_entry(statement, statement->assignment.target);
            printf("POPS LF@%s_%zx\n", statement->assignment.target, (uintptr_t)entry);
            break;
        case AST_VAR_DEF:
            generate_expression(statement->vardef.expression);
            SymtableEntry* entry_var_def = get_closest_symtable_entry(statement, statement->vardef.identifier);
            entry_var_def->defined = true;
            printf("POPS LF@%s_%zx\n", statement->vardef.identifier, (uintptr_t)entry_var_def);
            break;
        case AST_WHILE_DO: {
            static int while_counter = 0;
            while_counter += 1;
            printf("LABEL while_start_%d\n", while_counter);
            printf("PUSHS bool@false\n");
            generate_expression(statement->while_do.condition);
            to_bool();
            printf("JUMPIFEQS while_end_%d\n", while_counter);
            generate_statements(statement->while_do.statements.first, false);
            printf("JUMP while_start_%d\n", while_counter);
            printf("LABEL while_end_%d\n", while_counter);
            break;
        }
        case AST_RETURN:
            generate_expression(statement->return_st.expression);
            printf("POPFRAME\n");
            printf("RETURN\n");
            break;
        case AST_FUNCTION:
            printf("CREATEFRAME\n");
            ASTNode* arg = statement->function.parameter_groups.first;
            while (arg != NULL) {
                generate_expression(arg);
                arg = arg->next;
                printf("CALL %s\n", statement->function.identifier);
            }
            printf("CALL %s\n", statement->function.identifier);
            break;
        case AST_TERM:
        case AST_LITERAL:
        case AST_EXPRESSION:
        case AST_FUNCALL:
        case AST_BUILTIN_FUNCALL:
        case AST_VAR_USE:
        case AST_INTERPOLATED_STRING:
            generate_expression(statement);
            if (!is_last) {
                printf("POPS LF@$discard\n");
            }
            break;
        case AST_IF: {
            static int if_counter = 0;
            if_counter += 1;
            int current_if = if_counter;
            printf("PUSHS bool@false\n");
            generate_expression(statement->if_statement.condition);
            to_bool();
            printf("JUMPIFEQS if_else_%d\n", current_if);
            generate_statements(statement->if_statement.then_node, false);
            printf("JUMP if_end_%d\n", current_if);
            printf("LABEL if_else_%d\n", current_if);
            if (statement->if_statement.else_node != NULL)
                generate_statements(statement->if_statement.else_node, false);
            printf("LABEL if_end_%d\n", current_if);
            break;
        }
        case AST_ELSE:
            generate_statements(statement->else_statement.statements.first, false);
            break;
        case AST_THEN:
            generate_statements(statement->then_statement.statements.first, false);
            break;
        default:
            fprintf(stderr, "generate_statements_switch: accessed wrong statement type %d\n", statement->type);
            exit(99);
    }
}

void generate_statements(ASTNode* statement, bool is_function_body)
{
    bool is_last = false;
    while (statement != NULL) {
        if (statement->next == NULL && is_function_body == true) {
            is_last = true;
        }
        generate_statements_switch(statement, is_last);
        statement = statement->next;
    }
}

bool is_string_expr(ASTNode* node)
{
    if (node == NULL)
        return false;

    if (node->type == AST_EXPRESSION) {
        if (node->expression.result_type == 's')
            return true;
        return false;
    }
    if (node->type == AST_LITERAL) {
        return node->literal.type == AST_STRING;
    }
    if (node->type == AST_INTERPOLATED_STRING) {
        return true;
    }
    if (node->type == AST_BUILTIN_FUNCALL && node->term.identifier != NULL) {
        return strcmp(node->term.identifier, "readLine") == 0 || strcmp(node->term.identifier, "chr") == 0 || strcmp(node->term.identifier, "subStr") == 0 || strcmp(node->term.identifier, "toStr") == 0;
    }
    if (node->type == AST_VAR_USE && node->term.identifier != NULL) {
        SymtableEntry* entry = get_closest_defined_symtable_entry(node, node->term.identifier);
        if (entry != NULL && entry->data_type == 's')
            return true;
    }
    return false;
}

void to_bool()
{
    int id = builtin_label_id++;
    printf("POPS LF@$op1\n");
    printf("TYPE LF@$tmp_type LF@$op1\n");

    printf("JUMPIFEQ tobool_false_%d LF@$tmp_type string@nil\n", id);
    printf("JUMPIFEQ tobool_bool_%d LF@$tmp_type string@bool\n", id);
    printf("JUMPIFEQ tobool_int_%d LF@$tmp_type string@int\n", id);
    printf("JUMPIFEQ tobool_float_%d LF@$tmp_type string@float\n", id);
    printf("JUMPIFEQ tobool_str_%d LF@$tmp_type string@string\n", id);

    printf("LABEL tobool_bool_%d\n", id);
    printf("JUMPIFEQ tobool_false_%d bool@false LF@$op1\n", id);
    printf("JUMP tobool_true_%d\n", id);
    printf("LABEL tobool_int_%d\n", id);
    printf("JUMPIFEQ tobool_false_%d int@0 LF@$op1\n", id);
    printf("JUMP tobool_true_%d\n", id);
    printf("LABEL tobool_float_%d\n", id);
    printf("JUMPIFEQ tobool_false_%d float@0x0p+0 LF@$op1\n", id);
    printf("JUMP tobool_true_%d\n", id);
    printf("LABEL tobool_str_%d\n", id);
    printf("JUMPIFEQ tobool_false_%d string@ LF@$op1\n", id);
    printf("JUMP tobool_true_%d\n", id);

    printf("LABEL tobool_true_%d\n", id);
    printf("PUSHS bool@true\n");
    printf("JUMP tobool_end_%d\n", id);

    printf("LABEL tobool_false_%d\n", id);
    printf("PUSHS bool@false\n");
    printf("JUMP tobool_end_%d\n", id);
    printf("LABEL tobool_end_%d\n", id);
}

void to_str()
{
    int id = builtin_label_id++;
    printf("POPS LF@$op1\n");
    printf("TYPE LF@$tmp_type LF@$op1\n");
    printf("JUMPIFEQ $tostr_str_%d LF@$tmp_type string@string\n", id);
    printf("JUMPIFEQ $tostr_int_%d LF@$tmp_type string@int\n", id);
    printf("JUMPIFEQ $tostr_flt_%d LF@$tmp_type string@float\n", id);
    printf("JUMPIFEQ $tostr_bool_%d LF@$tmp_type string@bool\n", id);

    printf("MOVE LF@$op1 string@()\n");
    printf("JUMP $tostr_end_%d\n", id);
    printf("LABEL $tostr_int_%d\n", id);
    printf("INT2STR LF@$op1 LF@$op1\n");
    printf("JUMP $tostr_end_%d\n", id);
    printf("LABEL $tostr_flt_%d\n", id);
    printf("FLOAT2STR LF@$op1 LF@$op1\n");
    printf("JUMP $tostr_end_%d\n", id);
    printf("LABEL $tostr_bool_%d\n", id);
    printf("JUMPIFEQ $tostr_true_%d LF@$op1 bool@true\n", id);
    printf("MOVE LF@$op1 string@false\n");
    printf("JUMP $tostr_end_%d\n", id);
    printf("LABEL $tostr_true_%d\n", id);
    printf("MOVE LF@$op1 string@true\n");
    printf("JUMP $tostr_end_%d\n", id);
    printf("LABEL $tostr_str_%d\n", id);
    printf("LABEL $tostr_end_%d\n", id);
    printf("PUSHS LF@$op1\n");
}