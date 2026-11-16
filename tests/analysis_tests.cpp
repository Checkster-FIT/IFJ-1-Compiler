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

        testing::internal::CaptureStderr();
    }

    void TearDown() override
    {
        testing::internal::GetCapturedStderr();

        if (stream != nullptr)
            fclose(stream);

        free_ast_nodes(ast);
        free_strings(strings);
    }
};

// ==================================================================

TEST_F(analysis, dummy)
{
    const auto code = R"(
        import ifj26.*
        @main def function() =
            print(2);
    )";

    ASSERT_GE(fputs(code, stream), 0);
    rewind(stream);

    ASSERT_EQ(0, run_parser(ast, strings, &parser));
    EXPECT_EQ(0, run_semantic(ast));

    print_ast(ast);
}