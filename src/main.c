/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    main.c
 * @brief   The entry point for the compiler program
 * @author  Filip Hajdusek (xhajduf00)
 */

#include "generator.h"
#include "parser.h"
#include "semantic.h"
#include "shared_strings.h"

int main()
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

    const int exit_code_parser = run_parser(ast, strings, NULL);
    const int exit_code_semantic = run_semantic(ast);

    run_generator(ast);

    free_ast_nodes(ast);
    free_strings(strings);

    return exit_code_parser || exit_code_semantic;
}