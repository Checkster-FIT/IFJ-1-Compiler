/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator.c
 * @brief
 * @author
 */

#include "generator.h"
#include "ast.h"
#include "shared_data.h"

#include <stdio.h>

int run_generator(ASTNode* ast, const SharedData* data)
{
    (void) data;

    ASTNodeList* functions = &ast->program.functions;
    functions->active = functions->first;

    int counter = 0;
    fprintf(stderr, "\n");
    while (functions->active) {
        const ASTFunction* function = &functions->active->function;

        counter++;
        fprintf(stderr, "     %s", get_identifier(data, function->identifier_idx));

        if (function->is_main)
            fprintf(stderr, " (@main)");

        fprintf(stderr, "\n");
        functions->active = functions->active->next;
    }

    return counter;
}
