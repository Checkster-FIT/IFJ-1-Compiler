/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    ast.h
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_AST_H
#define IFJ_1_COMPILER_AST_H

#include <stdbool.h>

typedef struct ast_node ASTNode;

// ====================================

typedef enum {
    AST_NONE,
    AST_PROGRAM,
    AST_FUNCTION,
    AST_PARAMETER,
    AST_VAR_DEF,
    AST_VAR_ASSIGN,
    AST_VAR,
} ASTNodeType;

typedef struct {
    ASTNode* first;
    ASTNode* last;
    ASTNode* active;
} ASTNodeList;

// ====================================

typedef struct {
    ASTNodeList functions;
    ASTNode* recent_node;
} ASTProgram;

typedef struct {
    const char* identifier;
    bool is_main;
    ASTNodeList statements;
    ASTNodeList parameters;
} ASTFunction;

typedef struct {
    const char* identifier;
    ASTNode* expression;
} ASTVarDef;

typedef struct {
    const char* identifier;
} ASTVariable;

typedef struct {
    const char* identifier;
} ASTParameter;

typedef struct {
    ASTNode* target;
    ASTNode* expression;
} ASTAssignment;

// ====================================

struct ast_node {
    ASTNodeType type;
    ASTNode* next;

    union {
        ASTProgram program;
        ASTVarDef vardef;
        ASTFunction function;
        ASTParameter parameter;
        ASTVariable variable;
        ASTAssignment assignment;
    };
};

// =====================================

ASTNode* new_ast();
void destroy_ast(ASTNode* ast);

ASTNode* ast_add_function(ASTNode* ast, const char* identifier, bool main);
ASTNode* ast_add_parameter(ASTNode* ast, const char* identifier);
ASTNode* ast_add_vardef(ASTNode* ast, const char* identifier);

void ast_list_add(ASTNodeList* list, ASTNode* node);

#endif // IFJ_1_COMPILER_AST_H
