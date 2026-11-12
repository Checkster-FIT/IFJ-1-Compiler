/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    parser_tests.cpp
 * @brief   Automated unit tests for the parser module
 * @author  Filip Hajdusek (xhajduf00)
 */

#include <gtest/gtest.h>

extern "C" {
#include "parser.h"
}

// =====================================================

class parser : public ::testing::Test {
protected:
    ParserContext context = { };
    AllocatedStrings* strings = { };
    ASTNode* ast = { };
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

    EXPECT_EQ(nullptr, ast->program.functions.first);
    EXPECT_EQ(nullptr, ast->program.extensions.first);
    EXPECT_EQ(nullptr, ast->program.recent_function);
    EXPECT_EQ(nullptr, ast->program.recent_node);
    EXPECT_EQ(nullptr, top(ast->program.block_stack));
}

TEST_F(parser, empty)
{
    set_parser({ });
    ASSERT_EQ(2, run_parser(ast, strings, &context));

    EXPECT_EQ(nullptr, ast->program.functions.first);
    EXPECT_EQ(nullptr, ast->program.extensions.first);
    EXPECT_EQ(nullptr, ast->program.recent_function);
    EXPECT_EQ(nullptr, ast->program.recent_node);
    EXPECT_EQ(nullptr, top(ast->program.block_stack));
}

TEST_F(parser, function_single_line)
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

    ASTNode* function_node = ast->program.functions.first;
    ASSERT_NE(function_node, nullptr);
    EXPECT_EQ(function_node, ast->program.functions.last);
    EXPECT_EQ(function_node, ast->program.recent_function);

    EXPECT_EQ(nullptr, top(ast->program.block_stack));
    EXPECT_EQ(nullptr, ast->program.extensions.first);
}