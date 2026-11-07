/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    generator_tests.cpp
 * @brief   Automated unit tests for the generator module
 * @author  Tobias Mikan (xmikant00)
 */

#include <gtest/gtest.h>

extern "C" {
#include "ast.h"
#include "generator.h"
#include "symtable.h"
}

class Generator : public ::testing::Test {
protected:
    ASTNode* ast = nullptr;

    void SetUp() override { ast = new_ast(); }
    void TearDown() override { free_ast_nodes(ast); }

    void add_function(const char* name, bool is_main)
    {
        ast_add_function(ast, name);
        create_local_symtable(ast);   // jinak spadne free_symtable(NULL)

        if (is_main) {
            SymtableEntry e = {};
            e.defined = true;
            e.main = true;
            e.identifier = name;
            symtable_add(ast->program.global_symtable, e);
        }
    }
};

TEST_F(Generator, main_function)
{
    add_function("foo", false);
    add_function("bonq", false);
    add_function("main", true);

    testing::internal::CaptureStdout();
    generator(ast);
    std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("CALL $main\n"), std::string::npos);
    EXPECT_NE(out.find("LABEL $foo\n"), std::string::npos);
    EXPECT_NE(out.find("LABEL $bonq\n"), std::string::npos);
    EXPECT_NE(out.find("LABEL $main\n"), std::string::npos);
}

TEST_F(Generator, no_main_function)
{
    add_function("foo", false);
    EXPECT_EXIT(generator(ast), ::testing::ExitedWithCode(99),
        "entry function not found");
}