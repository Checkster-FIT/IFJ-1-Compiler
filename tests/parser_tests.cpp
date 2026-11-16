/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    parser_tests.cpp
 * @brief   Automated unit tests for the parser module
 * @author  Filip Hajdusek (xhajduf00)
 */

#include <gtest/gtest.h>

extern "C" {
#include "debug.h"
#include "parser.h"
}

// =====================================================

class parser : public ::testing::Test {
protected:
    ParserContext context = {};
    AllocatedStrings *strings = {};
    ASTNode *ast = {};
    std::vector<Token> tokens;

    static Token token(const type_enum type) {
        return {.type = type};
    }

    static Token token(const type_enum type, const char *value) {
        return {.type = type, .attribute_string = value};
    }

    static Token token(const type_enum type, const int value) {
        return {.type = type, .attribute_int = value};
    }

    static Token token(const type_enum type, const double value) {
        return {.type = type, .attribute_double = value};
    }

    static Token token(const type_enum type, const enum attribute_keyword keyword) {
        return {.type = type, .attribute_keyword = keyword};
    }

    static Token token(const type_enum type, const enum attribute_operation_type operation) {
        return {.type = type, .attribute_operation_type = operation};
    }

    void set_parser(std::vector<Token> token_list) {
        tokens = std::move(token_list);
        context.mockup_tokens = tokens.data();
        context.mockup_scanner = true;
        context.mockup_token_count = tokens.size();
        context.mockup_token_index = 0;
        context.debug = false;
    }

    void set_prepared_parser(const std::vector<Token> &token_list) {
        std::vector<Token> header = {
            token(KEYWORD, IMPORT_KW),
            token(IDENTIFIER, "ifj26"),
            token(DOT),
            token(OPERATOR, MULTIPLY),
            token(LINE_FEED),
        };

        header.insert(header.end(), token_list.begin(), token_list.end());
        set_parser(header);
    }

    void SetUp() override {
        strings = new_strings();
        ASSERT_NE(strings, nullptr);
        ast = new_ast();
        ASSERT_NE(ast, nullptr);

        ASSERT_NE(init_parser(&context, ast, strings), nullptr);
        testing::internal::CaptureStderr();
    }

    void TearDown() override {
        testing::internal::GetCapturedStderr();
        free_ast_nodes(ast);
        free_strings(strings);
    }

    void enable_debug() {
        context.debug = true;
        testing::internal::GetCapturedStderr();
    }

    static void display_ast(ASTNode *ast) {
        testing::internal::GetCapturedStderr();
        print_ast(ast);
        testing::internal::CaptureStderr();
    }
};

// ==================================================================

TEST_F(parser, header_only) {
    set_parser({
        token(KEYWORD, IMPORT_KW),
        token(IDENTIFIER, "ifj26"),
        token(DOT),
        token(OPERATOR, MULTIPLY),
        token(LINE_FEED),
    });

    // enable_debug();
    ASSERT_EQ(0, run_parser(ast, strings, &context));

    // ----------------------------------------------------
    EXPECT_EQ(nullptr, ast->program.functions.first);
    EXPECT_EQ(0, ast->program.functions.count);
    EXPECT_EQ(nullptr, ast->program.extensions.first);
    EXPECT_EQ(0, ast->program.extensions.count);

    EXPECT_EQ(nullptr, ast->program.recent_function);
    EXPECT_EQ(nullptr, ast->program.recent_node);
}

TEST_F(parser, empty) {
    set_parser({});
    ASSERT_EQ(2, run_parser(ast, strings, &context));

    // ----------------------------------------------------
    EXPECT_EQ(nullptr, ast->parent);
    EXPECT_EQ(nullptr, ast->program.functions.first);
    EXPECT_EQ(0, ast->program.functions.count);
    EXPECT_EQ(nullptr, ast->program.extensions.first);
    EXPECT_EQ(0, ast->program.extensions.count);
    EXPECT_EQ(nullptr, ast->program.recent_function);
    EXPECT_EQ(nullptr, ast->program.recent_node);
}

TEST_F(parser, function_expression) {
    set_prepared_parser({
        token(KEYWORD, DEF_KW),
        token(IDENTIFIER, "function"),
        token(LEFT_BRACKET),
        token(RIGHT_BRACKET),
        token(ASSIGNMENT),
        token(INTEGER, 1),
        token(LINE_FEED),
    });

    ASSERT_EQ(0, run_parser(ast, strings, &context));

    // ----------------------------------------------------
    ASTNode *function_node = ast->program.functions.first;
    ASSERT_NE(function_node, nullptr);
    ASSERT_EQ(function_node->next, nullptr);
    ASSERT_EQ(function_node->parent, ast);

    EXPECT_STREQ(function_node->function.identifier, "function");
    EXPECT_EQ(function_node, ast->program.functions.last);
    EXPECT_EQ(function_node, ast->program.recent_function);
    EXPECT_EQ(1, ast->program.functions.count);

    EXPECT_EQ(0, ast->program.extensions.count);
    EXPECT_EQ(nullptr, ast->program.extensions.first);
}

TEST_F(parser, function_indent_param) {
    set_prepared_parser({
        token(MAIN_IDENTIFIER),
        token(KEYWORD, DEF_KW),
        token(IDENTIFIER, "_example_fun2"),
        token(LEFT_BRACKET),
        token(IDENTIFIER, "param1"),
        token(DECLARATION_SIGN),
        token(KEYWORD, INT_KW),
        token(RIGHT_BRACKET),
        token(DECLARATION_SIGN),
        token(KEYWORD, INT_KW),
        token(ASSIGNMENT),
        token(LINE_FEED),
        token(INDENT),
        token(IDENTIFIER, "param1"),
        token(LINE_FEED),
        token(DEDENT),
    });

    ASSERT_EQ(0, run_parser(ast, strings, &context));

    // ----------------------------------------------------
    ASTNode *function_node = ast->program.functions.first;
    ASSERT_NE(function_node, nullptr);
    ASSERT_EQ(function_node->type, AST_FUNCTION);
    ASSERT_EQ(function_node->next, nullptr);
    ASSERT_EQ(function_node->parent, ast);

    EXPECT_STREQ(function_node->function.identifier, "_example_fun2");
    EXPECT_EQ(function_node, ast->program.functions.last);
    EXPECT_EQ(function_node, ast->program.recent_function);
    EXPECT_EQ(1, ast->program.functions.count);

    // ----------------------------------------------------
    ASTNode *param_group_node = function_node->function.parameter_groups.first;
    ASSERT_NE(param_group_node, nullptr);
    ASSERT_EQ(param_group_node->type, AST_PARAMETER_GROUP);

    EXPECT_EQ(param_group_node, function_node->function.parameter_groups.last);
    EXPECT_EQ(1, function_node->function.parameter_groups.count);

    // ----------------------------------------------------
    ASTNode *parameter_node = param_group_node->parameter_group.parameters.first;
    ASSERT_NE(parameter_node, nullptr);
    ASSERT_EQ(parameter_node->type, AST_PARAMETER);

    EXPECT_EQ(1, param_group_node->parameter_group.parameters.count);
    EXPECT_EQ(parameter_node, param_group_node->parameter_group.parameters.last);
    EXPECT_STREQ(parameter_node->parameter.identifier, "param1");

    // Symtable checks
    // ----------------------------------------------------
    SymtableEntry *param_entry = get_closest_symtable_entry(parameter_node, parameter_node->parameter.identifier);
    ASSERT_NE(param_entry, nullptr);
    EXPECT_STREQ(param_entry->identifier, "param1");
    EXPECT_EQ(param_entry->parameters, nullptr);
    EXPECT_EQ(param_entry->data_type, 'i');
    EXPECT_EQ(param_entry->builtin, false);
    EXPECT_EQ(param_entry->function, false);
    EXPECT_EQ(param_entry->main, false);
    EXPECT_EQ(param_entry->constant, true);
    EXPECT_EQ(param_entry->defined, true);

    // ----------------------------------------------------
    SymtableEntry *function_entry = get_closest_symtable_entry(function_node, function_node->function.identifier);
    ASSERT_NE(function_entry, nullptr);
    EXPECT_STREQ(function_entry->identifier, "_example_fun2");
    EXPECT_EQ(function_entry->data_type, 'i');
    EXPECT_EQ(function_entry->builtin, false);
    EXPECT_EQ(function_entry->function, true);
    EXPECT_EQ(function_entry->main, true);
    EXPECT_EQ(function_entry->constant, true);
    EXPECT_EQ(function_entry->defined, true);

    ASSERT_NE(function_entry->parameters, nullptr);
    ASSERT_NE(function_entry->parameters->chars, nullptr);
    EXPECT_STREQ(function_entry->parameters->chars, "#i");

    // ----------------------------------------------------
    EXPECT_EQ(nullptr, ast->program.extensions.first);
}

TEST_F(parser, functin_all_types) {
    set_prepared_parser({
        token(KEYWORD, DEF_KW),
        token(IDENTIFIER, "function_all"),
        token(LEFT_BRACKET),
        token(IDENTIFIER, "param_i"),
        token(DECLARATION_SIGN),
        token(KEYWORD, INT_KW),
        token(COMMA),
        token(IDENTIFIER, "param_d"),
        token(DECLARATION_SIGN),
        token(KEYWORD, DOUBLE_KW),
        token(COMMA),
        token(IDENTIFIER, "param_s"),
        token(DECLARATION_SIGN),
        token(KEYWORD, STRING_KW),
        token(COMMA),
        token(IDENTIFIER, "param_b"),
        token(DECLARATION_SIGN),
        token(KEYWORD, BOOLEAN_KW),
        token(COMMA),
        token(IDENTIFIER, "param_u"),
        token(DECLARATION_SIGN),
        token(KEYWORD, UNIT_KW),
        token(RIGHT_BRACKET),
        token(ASSIGNMENT),
        token(LINE_FEED),
        token(INDENT),
        // Variable - Int
        token(KEYWORD, VAR_KW),
        token(IDENTIFIER, "var_i"),
        token(DECLARATION_SIGN),
        token(KEYWORD, INT_KW),
        token(ASSIGNMENT),
        token(IDENTIFIER, "param_i"),
        token(LINE_FEED),
        // Variable - Double
        token(KEYWORD, VAR_KW),
        token(IDENTIFIER, "var_d"),
        token(DECLARATION_SIGN),
        token(KEYWORD, DOUBLE_KW),
        token(ASSIGNMENT),
        token(IDENTIFIER, "param_d"),
        token(LINE_FEED),
        // Variable - String
        token(KEYWORD, VAR_KW),
        token(IDENTIFIER, "var_s"),
        token(DECLARATION_SIGN),
        token(KEYWORD, STRING_KW),
        token(ASSIGNMENT),
        token(IDENTIFIER, "param_s"),
        token(LINE_FEED),
        // Variable - Boolean
        token(KEYWORD, VAR_KW),
        token(IDENTIFIER, "var_b"),
        token(DECLARATION_SIGN),
        token(KEYWORD, BOOLEAN_KW),
        token(ASSIGNMENT),
        token(IDENTIFIER, "param_b"),
        token(LINE_FEED),
        // Variable - Int
        token(KEYWORD, VAR_KW),
        token(IDENTIFIER, "var_u"),
        token(DECLARATION_SIGN),
        token(KEYWORD, UNIT_KW),
        token(ASSIGNMENT),
        token(IDENTIFIER, "param_u"),
        token(LINE_FEED),
        token(DEDENT),
    });

    ASSERT_EQ(0, run_parser(ast, strings, &context));

    // ----------------------------------------------------
    ASTNode *function_node = ast->program.functions.first;
    ASSERT_EQ(function_node->type, AST_FUNCTION);
    ASSERT_NE(function_node, nullptr);
    ASSERT_EQ(function_node->next, nullptr);
    ASSERT_EQ(function_node->parent, ast);

    EXPECT_STREQ(function_node->function.identifier, "function_all");
    EXPECT_EQ(function_node, ast->program.functions.last);
    EXPECT_EQ(function_node, ast->program.recent_function);
    EXPECT_EQ(1, ast->program.functions.count);

    EXPECT_EQ(0, ast->program.extensions.count);
    EXPECT_EQ(nullptr, ast->program.extensions.first);

    // ---------------------------------------------------
    ASSERT_EQ(1, function_node->function.parameter_groups.count);
    ASSERT_EQ(5, function_node->function.parameter_groups.first->parameter_group.parameters.count);
    ASSERT_EQ(5, function_node->function.statements.count);

    ASSERT_NE(nullptr, function_node->function.symtable);
    ASSERT_NE(nullptr, search(function_node->function.symtable, "param_i"));
    ASSERT_NE(nullptr, search(function_node->function.symtable, "param_d"));
    ASSERT_NE(nullptr, search(function_node->function.symtable, "param_s"));
    ASSERT_NE(nullptr, search(function_node->function.symtable, "param_b"));
    ASSERT_NE(nullptr, search(function_node->function.symtable, "param_u"));
    ASSERT_NE(nullptr, search(function_node->function.symtable, "var_i"));
    ASSERT_NE(nullptr, search(function_node->function.symtable, "var_d"));
    ASSERT_NE(nullptr, search(function_node->function.symtable, "var_s"));
    ASSERT_NE(nullptr, search(function_node->function.symtable, "var_b"));
    ASSERT_NE(nullptr, search(function_node->function.symtable, "var_u"));
}

void check_built_in_functions(const ASTNode *node, const char *identifier, const char return_type, const char *params) {
    SymtableEntry *entry = get_closest_symtable_entry(node, identifier);
    ASSERT_NE(entry, nullptr);
    ASSERT_STREQ(entry->identifier, identifier);
    EXPECT_EQ(entry->data_type, return_type);
    EXPECT_EQ(entry->builtin, true);
    EXPECT_EQ(entry->function, true);
    EXPECT_EQ(entry->main, false);
    EXPECT_EQ(entry->constant, true);
    EXPECT_EQ(entry->defined, true);

    ASSERT_NE(entry->parameters, nullptr);
    ASSERT_NE(entry->parameters->chars, nullptr);
    EXPECT_STREQ(entry->parameters->chars, params);
}

TEST_F(parser, built_in_functions) {
    set_prepared_parser({
        token(MAIN_IDENTIFIER),
        token(KEYWORD, DEF_KW),
        token(IDENTIFIER, "user_defined"),
        token(LEFT_BRACKET),
        token(RIGHT_BRACKET),
        token(ASSIGNMENT),
        token(DOUBLE, 3.6),
        token(LINE_FEED),
    });

    ASSERT_EQ(0, run_parser(ast, strings, &context));

    // ----------------------------------------------------
    ASTNode *function_node = ast->program.functions.first;
    ASSERT_EQ(ast->program.functions.count, 1);
    ASSERT_NE(function_node, nullptr);
    ASSERT_EQ(function_node->type, AST_FUNCTION);
    ASSERT_STREQ(function_node->function.identifier, "user_defined");

    // ----------------------------------------------------
    check_built_in_functions(function_node, "print", 'u', "#a");
    check_built_in_functions(function_node, "readLine", 's', "#");
    check_built_in_functions(function_node, "readInt", 'i', "#");
    check_built_in_functions(function_node, "readDouble", 'd', "#");
    check_built_in_functions(function_node, "toStr", 's', "#a");
    check_built_in_functions(function_node, "toInt", 'i', "#d");
    check_built_in_functions(function_node, "strLength", 'i', "#s");
    check_built_in_functions(function_node, "subStr", 's', "#sii");
    check_built_in_functions(function_node, "ord", 'i', "#si");
    check_built_in_functions(function_node, "chr", 's', "#i");

    check_built_in_functions(function_node, "readBoolean", 'b', "#");

    check_built_in_functions(function_node, "a%toString", 's', "");
    check_built_in_functions(function_node, "d%toInt", 'i', "");
    check_built_in_functions(function_node, "d%floor", 'd', "");
    check_built_in_functions(function_node, "s%length", 'i', "");
}

TEST_F(parser, var_def) {
    set_prepared_parser({
        // Function definition
        token(MAIN_IDENTIFIER),
        token(KEYWORD, DEF_KW),
        token(IDENTIFIER, "var_def_function"),
        token(LEFT_BRACKET),
        token(IDENTIFIER, "str"),
        token(DECLARATION_SIGN),
        token(KEYWORD, STRING_KW),
        token(RIGHT_BRACKET),
        token(DECLARATION_SIGN),
        token(KEYWORD, INT_KW),
        token(ASSIGNMENT),
        token(LINE_FEED),
        token(INDENT),
        // Function body
        token(KEYWORD, VAR_KW),
        token(IDENTIFIER, "variable"),
        token(ASSIGNMENT),
        token(IDENTIFIER, "str"),
        token(LINE_FEED),
        token(KEYWORD, VAL_KW),
        token(IDENTIFIER, "value"),
        token(ASSIGNMENT),
        token(IDENTIFIER, "str"),
        token(LINE_FEED),
        token(DEDENT),
    });

    ASSERT_EQ(0, run_parser(ast, strings, &context));

    // ----------------------------------------------------

    ASTNode *function_node = ast->program.functions.first;
    ASSERT_NE(function_node, nullptr);
    ASSERT_EQ(function_node->type, AST_FUNCTION);
    ASSERT_EQ(function_node->next, nullptr);
    ASSERT_EQ(function_node->parent, ast);

    EXPECT_STREQ(function_node->function.identifier, "var_def_function");
    EXPECT_EQ(function_node, ast->program.functions.last);
    EXPECT_EQ(function_node, ast->program.recent_function);

    ASSERT_EQ(1, ast->program.functions.count);
    ASSERT_EQ(2, function_node->function.statements.count);

    // ----------------------------------------------------
    ASTNode *var_node = function_node->function.statements.first;
    ASSERT_NE(var_node, nullptr);
    ASSERT_EQ(var_node->type, AST_VAR_DEF);
    ASSERT_EQ(var_node->parent, function_node);

    ASSERT_STREQ(var_node->vardef.identifier, "variable");
    ASSERT_NE(var_node->vardef.expression, nullptr);

    // ----------------------------------------------------
    ASTNode *val_node = var_node->next;
    ASSERT_NE(val_node, nullptr);
    ASSERT_EQ(val_node->type, AST_VAR_DEF);
    ASSERT_EQ(val_node->next, nullptr);
    ASSERT_EQ(val_node->parent, function_node);

    ASSERT_STREQ(val_node->vardef.identifier, "value");
    ASSERT_NE(val_node->vardef.expression, nullptr);

    // ----------------------------------------------------
    ASTNode *var_expr_node = var_node->vardef.expression;
    ASSERT_NE(var_expr_node, nullptr);
    ASSERT_EQ(var_expr_node->type, AST_TERM);
    ASSERT_EQ(var_expr_node->next, nullptr);
    ASSERT_EQ(var_expr_node->parent, var_node);
    ASSERT_EQ(var_expr_node->term.argument_groups.first, nullptr);
    ASSERT_STREQ(var_expr_node->term.identifier, "str");

    // ----------------------------------------------------
    ASTNode *val_expr_node = val_node->vardef.expression;
    ASSERT_NE(val_expr_node, nullptr);
    ASSERT_EQ(val_expr_node->type, AST_TERM);
    ASSERT_EQ(val_expr_node->next, nullptr);
    ASSERT_EQ(val_expr_node->parent, val_node);
    ASSERT_EQ(val_expr_node->term.argument_groups.first, nullptr);
    ASSERT_STREQ(val_expr_node->term.identifier, "str");
}

TEST_F(parser, var_def_function_call) {
    set_prepared_parser({
        // Function definition
        token(MAIN_IDENTIFIER),
        token(KEYWORD, DEF_KW),
        token(IDENTIFIER, "var_def_function"),
        token(LEFT_BRACKET),
        token(IDENTIFIER, "str"),
        token(DECLARATION_SIGN),
        token(KEYWORD, STRING_KW),
        token(RIGHT_BRACKET),
        token(DECLARATION_SIGN),
        token(KEYWORD, INT_KW),
        token(ASSIGNMENT),
        token(LINE_FEED),
        token(INDENT),
        // Function body
        token(KEYWORD, VAR_KW),
        token(IDENTIFIER, "variable"),
        token(ASSIGNMENT),
        token(IDENTIFIER, "call"),
        token(LEFT_BRACKET),
        token(IDENTIFIER, "str"),
        token(RIGHT_BRACKET),
        token(LINE_FEED),
        token(KEYWORD, VAL_KW),
        token(IDENTIFIER, "value"),
        token(ASSIGNMENT),
        token(IDENTIFIER, "call_2"),
        token(LEFT_BRACKET),
        token(RIGHT_BRACKET),
        token(LINE_FEED),
        token(DEDENT),
    });

    ASSERT_EQ(0, run_parser(ast, strings, &context));
    ASSERT_EQ(1, ast->program.functions.count);
    ASSERT_EQ(2, ast->program.functions.first->function.statements.count);
}

TEST_F(parser, var_def_invalid) {
    set_prepared_parser({
        // Function definition
        token(MAIN_IDENTIFIER),
        token(KEYWORD, DEF_KW),
        token(IDENTIFIER, "var_def_function"),
        token(LEFT_BRACKET),
        token(IDENTIFIER, "str"),
        token(DECLARATION_SIGN),
        token(KEYWORD, STRING_KW),
        token(RIGHT_BRACKET),
        token(DECLARATION_SIGN),
        token(KEYWORD, INT_KW),
        token(ASSIGNMENT),
        token(LINE_FEED),
        token(INDENT),
        // Function body
        token(KEYWORD, VAR_KW),
        token(IDENTIFIER, "variable"),
        token(LINE_FEED),
        token(KEYWORD, VAL_KW),
        token(IDENTIFIER, "value"),
        token(LINE_FEED),
        token(DEDENT),
    });

    ASSERT_EQ(2, run_parser(ast, strings, &context));
}

TEST_F(parser, semicolon) {
    set_prepared_parser({
        // Function definition
        token(MAIN_IDENTIFIER),
        token(KEYWORD, DEF_KW),
        token(IDENTIFIER, "var_def_function"),
        token(LEFT_BRACKET),
        token(IDENTIFIER, "str"),
        token(DECLARATION_SIGN),
        token(KEYWORD, STRING_KW),
        token(RIGHT_BRACKET),
        token(DECLARATION_SIGN),
        token(KEYWORD, INT_KW),
        token(ASSIGNMENT),
        token(LINE_FEED),
        token(INDENT),
        // Function body
        token(KEYWORD, VAR_KW),
        token(IDENTIFIER, "variable"),
        token(ASSIGNMENT),
        token(IDENTIFIER, "str"),
        token(SEMICOLON),
        token(KEYWORD, VAL_KW),
        token(IDENTIFIER, "value"),
        token(ASSIGNMENT),
        token(IDENTIFIER, "str"),
        token(SEMICOLON),
        token(LINE_FEED),
        // token(SEMICOLON),  // TODO: Empty expression?
        token(DEDENT),
    });

    ASSERT_EQ(0, run_parser(ast, strings, &context));
    ASSERT_EQ(1, ast->program.functions.count);
    ASSERT_EQ(2, ast->program.functions.first->function.statements.count);
}

TEST_F(parser, if_then_else) {
    set_prepared_parser({
        // Function definition
        token(MAIN_IDENTIFIER),
        token(KEYWORD, DEF_KW),
        token(IDENTIFIER, "if_then_else_function"),
        token(LEFT_BRACKET),
        token(IDENTIFIER, "bool"),
        token(DECLARATION_SIGN),
        token(KEYWORD, BOOLEAN_KW),
        token(RIGHT_BRACKET),
        token(DECLARATION_SIGN),
        token(KEYWORD, INT_KW),
        token(ASSIGNMENT),
        token(LINE_FEED),
        token(INDENT),
        // Function body
        token(KEYWORD, IF_KW),
        token(IDENTIFIER, "bool"),
        token(KEYWORD, THEN_KW),
        token(LINE_FEED),
        token(INDENT),
        token(INTEGER, 50),
        token(LINE_FEED),
        token(DEDENT),
        token(KEYWORD, ELSE_KW),
        token(LINE_FEED),
        token(INDENT),
        token(INTEGER, 100),
        token(LINE_FEED),
        token(DEDENT),
        token(DEDENT),
    });

    ASSERT_EQ(0, run_parser(ast, strings, &context));
    ASSERT_EQ(1, ast->program.functions.count);
    ASSERT_EQ(1, ast->program.functions.first->function.statements.count);

    // ----------------------------------------------------
    ASTNode *function_node = ast->program.functions.first;
    ASSERT_NE(function_node, nullptr);
    ASSERT_EQ(function_node->type, AST_FUNCTION);
    ASSERT_EQ(function_node->next, nullptr);
    ASSERT_EQ(function_node->parent, ast);

    EXPECT_STREQ(function_node->function.identifier, "if_then_else_function");
    EXPECT_EQ(function_node, ast->program.functions.last);
    EXPECT_EQ(function_node, ast->program.recent_function);

    // ----------------------------------------------------
    ASTNode *if_node = function_node->function.statements.first;
    ASSERT_NE(if_node, nullptr);
    ASSERT_EQ(if_node->type, AST_IF);
    ASSERT_EQ(if_node->next, nullptr);
    ASSERT_EQ(if_node->parent, function_node);

    ASSERT_NE(if_node->if_statement.condition, nullptr);
    ASSERT_NE(if_node->if_statement.then_node, nullptr);
    ASSERT_NE(if_node->if_statement.else_node, nullptr);

    ASTNode *condition_node = if_node->if_statement.condition;
    ASTNode *then_node = if_node->if_statement.then_node;
    ASTNode *else_node = if_node->if_statement.else_node;

    ASSERT_EQ(condition_node->parent, if_node);
    ASSERT_EQ(then_node->parent, if_node);
    ASSERT_EQ(else_node->parent, if_node);

    ASSERT_EQ(condition_node->type, AST_TERM);
    EXPECT_EQ(condition_node->term.argument_groups.count, 0);
    EXPECT_STREQ(condition_node->term.identifier, "bool");

    ASSERT_EQ(then_node->type, AST_THEN);
    EXPECT_EQ(then_node->then_statement.statements.count, 1);
    ASSERT_NE(then_node->then_statement.symtable, nullptr);

    ASTNode *statement_1 = then_node->then_statement.statements.first;
    ASSERT_NE(statement_1, nullptr);
    EXPECT_EQ(statement_1->type, AST_LITERAL);
    EXPECT_EQ(statement_1->literal.type, AST_INTEGER);
    EXPECT_EQ(statement_1->literal.val_integer, 50);

    ASSERT_EQ(else_node->type, AST_ELSE);
    EXPECT_EQ(else_node->else_statement.statements.count, 1);
    ASSERT_NE(else_node->else_statement.symtable, nullptr);

    ASTNode *statement_2 = else_node->else_statement.statements.first;
    ASSERT_NE(statement_2, nullptr);
    EXPECT_EQ(statement_2->type, AST_LITERAL);
    EXPECT_EQ(statement_2->literal.type, AST_INTEGER);
    EXPECT_EQ(statement_2->literal.val_integer, 100);
}

TEST_F(parser, while_do) {
    set_prepared_parser({
        // Function definition
        token(MAIN_IDENTIFIER),
        token(KEYWORD, DEF_KW),
        token(IDENTIFIER, "while_do_function"),
        token(LEFT_BRACKET),
        token(IDENTIFIER, "value"),
        token(DECLARATION_SIGN),
        token(KEYWORD, DOUBLE_KW),
        token(RIGHT_BRACKET),
        token(DECLARATION_SIGN),
        token(KEYWORD, UNIT_KW),
        token(ASSIGNMENT),
        token(LINE_FEED),
        token(INDENT),
        // Function body
        token(KEYWORD, WHILE_KW),
        token(IDENTIFIER, "value"),
        token(COMPARE, LESS),
        token(DOUBLE, 3.14159265359),
        token(KEYWORD, DO_KW),
        token(LINE_FEED),
        token(INDENT),
        token(IDENTIFIER, "value"),
        token(ASSIGNMENT),
        token(IDENTIFIER, "value"),
        token(OPERATOR, PLUS),
        token(INTEGER, 1),
        token(LINE_FEED),
        token(DEDENT),
        token(DEDENT),
    });

    ASSERT_EQ(0, run_parser(ast, strings, &context));
    ASSERT_EQ(1, ast->program.functions.count);
    ASSERT_EQ(1, ast->program.functions.first->function.statements.count);

    // ----------------------------------------------------
    ASTNode *function_node = ast->program.functions.first;
    ASSERT_NE(function_node, nullptr);
    ASSERT_EQ(function_node->type, AST_FUNCTION);
    ASSERT_EQ(function_node->next, nullptr);
    ASSERT_EQ(function_node->parent, ast);

    EXPECT_STREQ(function_node->function.identifier, "while_do_function");
    EXPECT_EQ(function_node, ast->program.functions.last);
    EXPECT_EQ(function_node, ast->program.recent_function);

    // ----------------------------------------------------
    ASTNode *while_node = function_node->function.statements.first;
    ASSERT_NE(while_node, nullptr);
    ASSERT_EQ(while_node->type, AST_WHILE_DO);
    ASSERT_EQ(while_node->next, nullptr);
    ASSERT_EQ(while_node->parent, function_node);

    ASSERT_NE(while_node->while_do.condition, nullptr);
    ASSERT_NE(while_node->while_do.statements.first, nullptr);
    ASSERT_NE(while_node->while_do.symtable, nullptr);

    ASTNode *condition_node = while_node->while_do.condition;
    ASTNode *assignment_node = while_node->while_do.statements.first;

    // Condition
    ASSERT_EQ(condition_node->parent, while_node);
    ASSERT_EQ(condition_node->type, AST_EXPRESSION);
    ASSERT_EQ(condition_node->expression.operation, OP_LESS);
    ASSERT_NE(condition_node->expression.left, nullptr);
    ASSERT_NE(condition_node->expression.right, nullptr);

    ASSERT_EQ(condition_node->expression.left->type, AST_TERM);
    ASSERT_EQ(condition_node->expression.left->term.argument_groups.count, 0);
    ASSERT_STREQ(condition_node->expression.left->term.identifier, "value");

    ASSERT_EQ(condition_node->expression.right->type, AST_LITERAL);
    ASSERT_EQ(condition_node->expression.right->literal.type, AST_DOUBLE);
    ASSERT_EQ(condition_node->expression.right->literal.val_double, 3.14159265359);

    // Assignment
    ASSERT_EQ(assignment_node->parent, while_node);
    ASSERT_EQ(assignment_node->type, AST_VAR_ASSIGN);
    ASSERT_STREQ(assignment_node->assignment.target, "value");
    ASSERT_NE(assignment_node->assignment.expression, nullptr);

    ASTNode *assigned_expression_node = assignment_node->assignment.expression;
    ASSERT_EQ(assigned_expression_node->parent, assignment_node);
    ASSERT_EQ(assigned_expression_node->type, AST_EXPRESSION);
    ASSERT_EQ(assigned_expression_node->expression.operation, OP_ADD);
    ASSERT_NE(assigned_expression_node->expression.left, nullptr);
    ASSERT_NE(assigned_expression_node->expression.right, nullptr);

    ASSERT_EQ(assigned_expression_node->expression.left->type, AST_TERM);
    ASSERT_EQ(assigned_expression_node->expression.left->term.argument_groups.count, 0);
    ASSERT_STREQ(assigned_expression_node->expression.left->term.identifier, "value");

    ASSERT_EQ(assigned_expression_node->expression.right->type, AST_LITERAL);
    ASSERT_EQ(assigned_expression_node->expression.right->literal.type, AST_INTEGER);
    ASSERT_EQ(assigned_expression_node->expression.right->literal.val_integer, 1);
}