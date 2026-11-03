/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    ast.h
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_AST_H
#define IFJ_1_COMPILER_AST_H

typedef enum {
    N_FUNCTION,
    N_DECLARATION,
    N_ASSIGNMENT,
} NodeType;

typedef struct ast_node {
    NodeType type;

    union {
        char* identifier;
        double val_double;
        int val_int;
    };

    struct ast_node* next;
} ASTNode;

typedef struct {
    ASTNode* first;
    ASTNode* active;
} NodeList;

typedef struct {
    NodeList functions;
    NodeList extensions;
} AbstractSyntaxTree;

void init_ast(AbstractSyntaxTree* ast);
void destroy_ast(AbstractSyntaxTree* ast);

ASTNode* add_function(AbstractSyntaxTree* ast, char* identifier);

#endif // IFJ_1_COMPILER_AST_H
