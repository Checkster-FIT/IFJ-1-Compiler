/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator.c
 * @brief
 * @author
 */

#include "generator.h"
#include "ast.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

int run_generator(ASTNode* ast)
{
    print_ast(ast);
    return 0;
}

void generator(ASTNode* node)
{
    printf(".IFJcode26\n\n");
    printf("CREATEFRAME\n");
    printf("PUSHFRAME\n");
    generate_program(node);
}

void generate_program(ASTNode* node)
{
    if (node == NULL || node->type != AST_PROGRAM) {
        fprintf(stderr, "generate_program: expected AST_PROGRAM\n");
        exit(99);
    }

    SymtableEntry* main_entry = symtable_main_search(node->program.global_symtable);
    if (main_entry == NULL) {
        fprintf(stderr, "generate_program: entry function not found\n");
        exit(99);
    }

    const char* main_name = main_entry->identifier;
    print_call(main_name);
    printf("EXIT int@0\n");

    ASTNode* helper_node = node->program.functions.first;
    while (helper_node != NULL) {
        generate_function(helper_node);
        helper_node = helper_node->next;
    }
}

void generate_function(ASTNode* node)
{
    print_label(node->function.identifier);
    printf("CREATEFRAME\n");
    printf("POPFRAME\n");
    printf("POPFRAME\n");
    printf("RETURN\n");
}

void print_call(const char* name)
{
    printf("CALL $%s\n", name);
}

void print_label(const char* name)
{
    printf("LABEL $%s\n", name);
}
