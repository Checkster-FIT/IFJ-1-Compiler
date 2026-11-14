/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    ast.h
 * @brief
 * @author  Filip Hajdusek (xhajduf00)
 */

#ifndef IFJ_1_COMPILER_AST_H
#define IFJ_1_COMPILER_AST_H

#include "stack.h"
#include "symtable.h"
#include <stdbool.h>

typedef struct ast_node ASTNode;

// ====================================

typedef enum {
    AST_NONE,
    AST_PROGRAM,

    AST_FUNCTION,
    AST_EXTENSION,
    AST_PARAMETER_GROUP,
    AST_PARAMETER,

    AST_EXPRESSION,
    AST_TERM,
    AST_LITERAL,
    AST_INTERPOLATED_STRING,
    AST_CONVERSION,

    AST_VAR_ASSIGN,
    AST_VAR_DEF,
    AST_RETURN,

    AST_WHILE_DO,
    AST_IF,
    AST_THEN,
    AST_ELSE,
} ASTNodeType;

typedef enum {
    AST_INTEGER,
    AST_DOUBLE,
    AST_BOOLEAN,
    AST_STRING,
    AST_UNIT,
    AST_UNKNOWN_LITERAL
} ASTLiteralType;

typedef struct {
    ASTNode* first;
    ASTNode* last;
    ASTNode* active;
} ASTNodeList;

typedef enum {
    AST_TARGET_FUNCTION,
    AST_TARGET_EXTENSION,
} ASTTarget;

// ====================================

typedef struct {
    ASTNodeList functions;
    ASTNodeList extensions;
    Symtable* global_symtable;

    ASTNode* recent_node;
    ASTNode* recent_function;
    Stack* block_stack;
} ASTProgram;

typedef struct {
    const char* identifier;
    ASTNodeList statements;
    ASTNodeList parameter_groups;
    Symtable* symtable;
} ASTFunction;

typedef struct {
    ASTNodeList parameters;
} ASTParamGroup;

typedef struct {
    ASTNode* condition;
    ASTNodeList statements;
    Symtable* symtable;
} ASTWhileDo;

typedef struct {
    ASTNode* condition;
    ASTNode* then_node;
    ASTNode* else_node;
} ASTIf;

typedef struct {
    ASTNodeList statements;
    Symtable* symtable;
} ASTThen;

typedef struct {
    ASTNodeList statements;
    Symtable* symtable;
} ASTElse;

typedef struct {
    ASTNodeList functions;
    ASTNodeList parameter_groups;
    Symtable* symtable;
} ASTExtension;

typedef struct {
    const char* identifier;
    ASTNode* expression;
} ASTVarDef;

typedef struct {
    ASTNode* expression;
} ASTReturn;

typedef struct {
    ASTNode* left;
    ASTNode* right;
    int operation;
    char result_type;
} ASTExpression;

typedef struct {
    char result_type;
    ASTNode* expression;
} ASTConversion;

typedef struct {
    const char* identifier;
} ASTString;

typedef struct {
    ASTLiteralType type;

    union {
        int val_integer;
        double val_double;
        bool val_bool;
        const char* val_string;
    };
} ASTLiteral;

typedef struct {
    ASTNodeList expressions;
} ASTInterpolatedString;

typedef struct {
    const char* identifier;
    ASTNodeList arguments;
} ASTTerm;

typedef struct {
    const char* identifier;
} ASTParameter;

typedef struct {
    const char* target;
    ASTNode* expression;
} ASTAssignment;

// ====================================

struct ast_node {
    ASTNodeType type;
    ASTNode* next;
    ASTNode* parent;
    int line;

    union {
        ASTProgram program;
        ASTVarDef vardef;
        ASTFunction function;
        ASTExtension extension;
        ASTParamGroup parameter_group;
        ASTParameter parameter;
        ASTAssignment assignment;
        ASTExpression expression;
        ASTLiteral literal;
        ASTWhileDo while_do;
        ASTReturn return_st;
        ASTTerm term;
        ASTInterpolatedString interpolated_string;
        ASTIf if_statement;
        ASTThen then_statement;
        ASTElse else_statement;
        ASTConversion conversion;
    };
};

// =====================================

typedef enum {
    OP_NONE,
    OP_DOT,
    OP_NOT,
    OP_UNARY_PLUS,
    OP_UNARY_MINUS,
    OP_MUL,
    OP_DIV,
    OP_ADD,
    OP_SUB,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_AND,
    OP_OR,
    OP_TERMINAL
} Operator;

// =====================================

ASTNode* new_ast();

void ast_list_add(ASTNodeList* list, ASTNode* node);

void free_ast_nodes(ASTNode* node);

void free_node_list(ASTNodeList* list);

Symtable* get_node_symtable(const ASTNode* node);

SymtableEntry* get_closest_symtable_entry(const ASTNode* node, const char* identifier);

SymtableEntry* get_closest_defined_symtable_entry(const ASTNode* node, const char* identifier);

ASTNodeList* get_top_statement_block(const ASTNode* ast);

ASTNode* ast_add_extension(ASTNode* ast, Symtable* symtable);

ASTNode* ast_add_function(ASTNode* ast, const char* identifier, Symtable* symtable, ASTTarget target);

ASTNode* ast_add_parameter_group(ASTNode* ast, const ASTTarget target);

ASTNode* ast_add_parameter(ASTNode* ast, const char* identifier, const ASTTarget target);

ASTNode* ast_add_vardef(ASTNode* ast, const char* identifier);

ASTNode* ast_add_assignment(ASTNode* ast, const char* identifier);

ASTNode* ast_add_return(ASTNode* ast);

ASTNode* ast_add_while_do(ASTNode* ast, Symtable* symtable);

ASTNode* ast_add_if(ASTNode* ast);

ASTNode* ast_add_if_branch(ASTNode* ast, ASTNodeType type, ASTNode* if_node, Symtable* symtable);

ASTNode* ast_add_expression(ASTNode* ast, ASTNode* expression);

ASTNode* ast_new_expression(ASTExpression expression);

ASTNode* ast_insert_conversion(ASTNode* node, char new_type);

ASTNode* ast_new_literal(ASTLiteral literal);

ASTNode* ast_new_term(const char* identifier);

ASTNode* ast_new_interpolated_string();

#endif // IFJ_1_COMPILER_AST_H
