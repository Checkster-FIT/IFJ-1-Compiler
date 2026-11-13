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
    AllocatedStrings* strings = {};
    ASTNode* ast = {};
    std::vector<Token> tokens;

    static Token token(const type_enum type)
    {
        return { .type = type };
    }
    static Token token(const type_enum type, const char* value)
    {
        return { .type = type, .attribute_string = value };
    }
    static Token token(const type_enum type, const int value)
    {
        return { .type = type, .attribute_int = value };
    }
    static Token token(const type_enum type, const double value)
    {
        return { .type = type, .attribute_double = value };
    }
    static Token token(const type_enum type, const enum attribute_keyword keyword)
    {
        return { .type = type, .attribute_keyword = keyword };
    }
    static Token token(const type_enum type, const enum attribute_operation_type operation)
    {
        return { .type = type, .attribute_operation_type = operation };
    }

    void set_parser(std::vector<Token> token_list)
    {
        tokens = std::move(token_list);
        context.mockup_tokens = tokens.data();
        context.mockup_scanner = true;
        context.mockup_token_count = tokens.size();
        context.mockup_token_index = 0;
        context.debug = false;
    }

    void set_prepared_parser(const std::vector<Token>& token_list)
    {
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

    void SetUp() override
    {
        strings = new_strings();
        ASSERT_NE(strings, nullptr);
        ast = new_ast();
        ASSERT_NE(ast, nullptr);

        ASSERT_NE(init_parser(&context, ast, strings), nullptr);
        testing::internal::CaptureStderr();
    }

    void TearDown() override
    {
        testing::internal::GetCapturedStderr();
        free_ast_nodes(ast);
        free_strings(strings);
    }

    void enable_debug()
    {
        context.debug = true;
        testing::internal::GetCapturedStderr();
    }

    static void display_ast(ASTNode* ast)
    {
        testing::internal::GetCapturedStderr();
        print_ast(ast);
        testing::internal::CaptureStderr();
    }
};

TEST_F(parser, header_only)
{
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
    EXPECT_EQ(nullptr, ast->program.extensions.first);
    EXPECT_EQ(nullptr, ast->program.recent_function);
    EXPECT_EQ(nullptr, ast->program.recent_node);
}

TEST_F(parser, empty)
{
    set_parser({});
    ASSERT_EQ(2, run_parser(ast, strings, &context));

    // ----------------------------------------------------
    EXPECT_EQ(nullptr, ast->parent);
    EXPECT_EQ(nullptr, ast->program.functions.first);
    EXPECT_EQ(nullptr, ast->program.extensions.first);
    EXPECT_EQ(nullptr, ast->program.recent_function);
    EXPECT_EQ(nullptr, ast->program.recent_node);
}

TEST_F(parser, function_expression)
{
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
    ASTNode* function_node = ast->program.functions.first;
    ASSERT_NE(function_node, nullptr);
    ASSERT_EQ(function_node->next, nullptr);
    ASSERT_EQ(function_node->parent, ast);

    EXPECT_STREQ(function_node->function.identifier, "function");
    EXPECT_EQ(function_node, ast->program.functions.last);
    EXPECT_EQ(function_node, ast->program.recent_function);

    EXPECT_EQ(nullptr, ast->program.extensions.first);
}

TEST_F(parser, function_indent_param)
{
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
    ASTNode* function_node = ast->program.functions.first;
    ASSERT_NE(function_node, nullptr);
    ASSERT_EQ(function_node->type, AST_FUNCTION);
    ASSERT_EQ(function_node->next, nullptr);
    ASSERT_EQ(function_node->parent, ast);

    EXPECT_STREQ(function_node->function.identifier, "_example_fun2");
    EXPECT_EQ(function_node, ast->program.functions.last);
    EXPECT_EQ(function_node, ast->program.recent_function);

    // ----------------------------------------------------
    ASTNode* param_group_node = function_node->function.parameter_groups.first;
    ASSERT_NE(param_group_node, nullptr);
    ASSERT_EQ(param_group_node->type, AST_PARAMETER_GROUP);

    EXPECT_EQ(param_group_node, function_node->function.parameter_groups.last);

    // ----------------------------------------------------
    ASTNode* parameter_node = param_group_node->parameter_group.parameters.first;
    ASSERT_NE(parameter_node, nullptr);
    ASSERT_EQ(parameter_node->type, AST_PARAMETER);

    EXPECT_EQ(parameter_node, param_group_node->parameter_group.parameters.last);
    EXPECT_STREQ(parameter_node->parameter.identifier, "param1");

    // Symtable checks
    // ----------------------------------------------------
    SymtableEntry* param_entry = get_closest_symtable_entry(parameter_node, parameter_node->parameter.identifier);
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
    SymtableEntry* function_entry = get_closest_symtable_entry(function_node, function_node->function.identifier);
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

void check_built_in_functions(const ASTNode* node, const char* identifier, const char return_type, const char* params)
{
    SymtableEntry* entry = get_closest_symtable_entry(node, identifier);
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

TEST_F(parser, built_in_functions)
{
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
    ASTNode* function_node = ast->program.functions.first;
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

    // check_built_in_functions(function_node, "toString", 's', "");
    // check_built_in_functions(function_node, "toInt", 'i', "");
    // check_built_in_functions(function_node, "floor", 'd', "");
    // check_built_in_functions(function_node, "length", 'i', "");
}

TEST_F(parser, var_def)
{
    set_prepared_parser({
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
    ASTNode* function_node = ast->program.functions.first;
    ASSERT_NE(function_node, nullptr);
    ASSERT_EQ(function_node->type, AST_FUNCTION);
    ASSERT_EQ(function_node->next, nullptr);
    ASSERT_EQ(function_node->parent, ast);

    EXPECT_STREQ(function_node->function.identifier, "var_def_function");
    EXPECT_EQ(function_node, ast->program.functions.last);
    EXPECT_EQ(function_node, ast->program.recent_function);

    // ----------------------------------------------------
    ASTNode* var_node = function_node->function.statements.first;
    ASSERT_NE(var_node, nullptr);
    ASSERT_EQ(var_node->type, AST_VAR_DEF);
    ASSERT_EQ(var_node->parent, function_node);

    ASSERT_STREQ(var_node->vardef.identifier, "variable");
    ASSERT_NE(var_node->vardef.expression, nullptr);

    // ----------------------------------------------------
    ASTNode* val_node = var_node->next;
    ASSERT_NE(val_node, nullptr);
    ASSERT_EQ(val_node->type, AST_VAR_DEF);
    ASSERT_EQ(val_node->next, nullptr);
    ASSERT_EQ(val_node->parent, function_node);

    ASSERT_STREQ(val_node->vardef.identifier, "value");
    ASSERT_NE(val_node->vardef.expression, nullptr);

    // ----------------------------------------------------
    ASTNode* var_expr_node = var_node->vardef.expression;
    ASSERT_NE(var_expr_node, nullptr);
    ASSERT_EQ(var_expr_node->type, AST_TERM);
    ASSERT_EQ(var_expr_node->next, nullptr);
    ASSERT_EQ(var_expr_node->parent, var_node);
    ASSERT_EQ(var_expr_node->term.arguments.first, nullptr);
    ASSERT_STREQ(var_expr_node->term.identifier, "str");

    // ----------------------------------------------------
    ASTNode* val_expr_node = val_node->vardef.expression;
    ASSERT_NE(val_expr_node, nullptr);
    ASSERT_EQ(val_expr_node->type, AST_TERM);
    ASSERT_EQ(val_expr_node->next, nullptr);
    ASSERT_EQ(val_expr_node->parent, val_node);
    ASSERT_EQ(val_expr_node->term.arguments.first, nullptr);
    ASSERT_STREQ(val_expr_node->term.identifier, "str");
}