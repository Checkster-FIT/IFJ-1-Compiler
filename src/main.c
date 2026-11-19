/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    main.c
 * @brief   The entry point for the compiler program
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "debug.h"
#include "generator.h"
#include "parser.h"
#include "semantic.h"
#include "shared_strings.h"

#include <string.h>

int main(const int argc, char* argv[])
{

    AllocatedStrings* strings = new_strings();
    if (strings == NULL) {
        return EXIT_CODE_INTERNAL_ERROR;
    }

    ASTNode* ast = new_ast();
    if (ast == NULL) {
        free_strings(strings);
        return EXIT_CODE_INTERNAL_ERROR;
    }

    // Syntax analysis
    const int exit_code_parser = run_parser(ast, strings, NULL);
    if (exit_code_parser != 0) {
        free_ast_nodes(ast);
        free_strings(strings);
        return exit_code_parser;
    }

    // Semantic analysis
    const int exit_code_semantic = run_semantic(ast);
    if (exit_code_semantic != 0) {
        free_ast_nodes(ast);
        free_strings(strings);
        return exit_code_semantic;
    }

    // Code generation
    generate_program(ast);

    // Flags
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--ast") == 0)
            print_ast(ast);
    }

    free_ast_nodes(ast);
    free_strings(strings);
    return 0;
}