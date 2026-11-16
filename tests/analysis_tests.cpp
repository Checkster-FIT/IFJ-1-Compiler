/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    analysis_tests.cpp
 * @brief   Automated complex tests for lexical, syntax and semantic analysis
 * @author  Filip Hajdusek (xhajduf00)
 */

#include <gtest/gtest.h>

extern "C" {
#include "debug.h"
#include "parser.h"
#include "scanner.h"
#include "semantic.h"
}

// =====================================================

class analysis : public ::testing::Test {
protected:
    ParserContext parser = { };
    AllocatedStrings* strings = { };
    ASTNode* ast = { };
    FILE* stream = nullptr;
    bool errors = false;

    void SetUp() override
    {
        // Shared struct for string allocation
        strings = new_strings();
        ASSERT_NE(strings, nullptr);
        // Program AST - modified by syntax and semantic analysis
        ast = new_ast();
        ASSERT_NE(ast, nullptr);
        // Temporary input stream
        stream = tmpfile();
        ASSERT_NE(stream, nullptr);

        ASSERT_NE(init_parser(&parser, ast, strings), nullptr);
        parser.input_stream = stream;
        parser.debug = false;

        testing::internal::CaptureStderr();
    }

    void TearDown() override
    {
        if (errors) {
            std::cout << testing::internal::GetCapturedStderr();
        } else {
            testing::internal::GetCapturedStderr();
        }

        if (stream != nullptr)
            fclose(stream);

        free_ast_nodes(ast);
        free_strings(strings);
    }

    void load_code(const char* code) const
    {
        ASSERT_GE(fputs(code, stream), 0);
        rewind(stream);
    }

    int check_syntax()
    {
        return run_parser(ast, strings, &parser);
    }

    int check_semantics() const
    {
        return run_semantic(ast);
    }

    int exit_code(const bool stderr = false)
    {
        if (stderr)
            errors = true;

        if (const int parser_error = check_syntax(); parser_error != 0)
            return parser_error;

        if (const int semantic_error = check_semantics(); semantic_error != 0)
            return semantic_error;

        return 0;
    }
};

// ==================================================================

TEST_F(analysis, empty_function)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit =
            ()
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, header_with_comments)
{
    load_code(R"(
        // This program has a comment before its header
        /* This program has a comment before its header */
        import ifj26.*
        @main def nothing(): Unit =
            ()
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, header_missing)
{
    load_code(R"(
        @main def nothing(): Unit =
            ()
    )");

    EXPECT_EQ(exit_code(), 2);
}

TEST_F(analysis, header_twice)
{
    load_code(R"(
        import ifj26.*
        import ifj26.*
        @main def nothing(): Unit =
            ()
    )");

    EXPECT_EQ(exit_code(), 2);
}

// =================================

TEST_F(analysis, main_newline)
{
    load_code(R"(
        import ifj26.*
        @main
        def nothing(): Unit =
            ()
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, main_missing)
{
    load_code(R"(
        import ifj26.*
        def nothing(): Unit =
            ()
    )");

    EXPECT_EQ(exit_code(), 3);
}

TEST_F(analysis, main_twice)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit =
            ()
        @main def something(): Unit =
            ()
    )");

    EXPECT_EQ(exit_code(), 4);
}

TEST_F(analysis, main_invalid_tag)
{
    load_code(R"(
        import ifj26.*
        @ main
        def nothing(): Unit =
            ()
    )");

    EXPECT_EQ(exit_code(), 2);
}

TEST_F(analysis, main_invalid_params)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(str: String): Unit =
            ()
    )");

    EXPECT_EQ(exit_code(), 5);
}

TEST_F(analysis, main_type_implicit)
{
    load_code(R"(
        import ifj26.*
        @main def nothing() =
            ()
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, main_type_implicit_incorrect)
{
    load_code(R"(
        import ifj26.*
        @main def nothing() =
            5
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, main_type_explicit)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit =
            5
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, main_type_explicit_incorrect)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Double =
            5.5
    )");

    EXPECT_EQ(exit_code(), 6);
}

// =================================

TEST_F(analysis, function_block_missing)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit =
    )");

    EXPECT_EQ(exit_code(), 2);
}

TEST_F(analysis, function_inline)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(s: String): String = s
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, function_identifier_missing)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def (s: String): String = s
    )");

    EXPECT_EQ(exit_code(), 2);
}

TEST_F(analysis, function_params_missing_comma)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(s: String i: Int): String = s
    )");

    EXPECT_EQ(exit_code(), 2);
}

TEST_F(analysis, function_params_missing_type)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(s): String = s
    )");

    EXPECT_EQ(exit_code(), 2);
}

TEST_F(analysis, function_params_missing_identifier)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(: String): String = s
    )");

    EXPECT_EQ(exit_code(), 2);
}

TEST_F(analysis, function_params_invalid_assignment)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(s: String): String =
            s = "something else";
    )");

    EXPECT_EQ(exit_code(), 7);
}