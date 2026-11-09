/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    ast.h
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_AST_H
#define IFJ_1_COMPILER_AST_H

#include "symtable.h"
#include <stdbool.h>

typedef struct ast_node ASTNode;

// ====================================

typedef enum {
    AST_NONE,
    AST_PROGRAM,
    AST_FUNCTION,
    AST_EXTENSION,
    AST_EXPRESSION,
    AST_PARAMETER,
    AST_VAR_ASSIGN,
    AST_VAR_DEF,
} ASTNodeType;

typedef struct {
    ASTNode* first;
    ASTNode* last;
    ASTNode* active;
} ASTNodeList;

// ====================================

typedef struct {
    ASTNodeList functions;
    ASTNodeList extensions;
    Symtable* global_symtable;

    ASTNode* recent_node;
    ASTNode* last_function;
} ASTProgram;

typedef struct {
    const char* identifier;
    ASTNodeList statements;
    ASTNodeList parameters;
    Symtable* symtable;
} ASTFunction;

typedef struct {
    ASTNodeList functions;
    ASTNode* parameter;
    Symtable* symtable;
} ASTExtension;

typedef struct {
    const char* identifier;
    ASTNode* expression;
} ASTVarDef;

typedef struct {
    ASTNode* left;
    ASTNode* right;
} ASTExpression;

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
        ASTExtension extension;
        ASTParameter parameter;
        ASTVariable variable;
        ASTAssignment assignment;
        ASTExpression expression;
    };
};

// =====================================

ASTNode* new_ast();
void free_ast_nodes(ASTNode* node);
void free_node_list(ASTNodeList* list);

ASTNode* ast_add_extension(ASTNode* ast, Symtable* symtable);
ASTNode* ast_add_ext_parameter(ASTNode* ast, const char* identifier);
ASTNode* ast_add_ext_function(ASTNode* ast, const char* identifier, Symtable* symtable);

ASTNode* ast_add_function(ASTNode* ast, const char* identifier, Symtable* symtable);
ASTNode* ast_add_parameter(ASTNode* ast, const char* identifier);

ASTNode* ast_add_vardef(ASTNode* ast, const char* identifier);
ASTNode* ast_new_expression(const ASTExpression expression);

Symtable* create_local_symtable(ASTNode* ast);

void ast_list_add(ASTNodeList* list, ASTNode* node);

#endif // IFJ_1_COMPILER_AST_H
