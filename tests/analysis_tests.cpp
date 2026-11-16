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

// ===============================================================================

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

// ========================================================

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

// ===========================================================

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

// ===========================================================

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

// ===========================================================

TEST_F(analysis, return_statement_invalid_implicit_type)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something() =
            return
    )");

    EXPECT_EQ(exit_code(), 10);
}

TEST_F(analysis, return_statement_invalid_explicit_type)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(): String =
            return
    )");

    EXPECT_EQ(exit_code(), 10);
}

// ===========================================================

TEST_F(analysis, return_value_valid)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(): String =
           "this is interpreted as String"
    )");
    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, return_value_valid_expression)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int): Int =
           i + 52
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, return_value_valid_expression_single_line)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int): Int = i + 52
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, return_value_valid_funcall)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(): Double =
           not_pi()
        def not_pi() = 3.14159265359
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, return_value_valid_funcall_single_line)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(): Double = not_pi()
        def not_pi() = 3.14159265359
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, return_value_valid_not_unit)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(): Unit = 55
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, return_value_after_vardef)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(): String =
            val str = "this is interpreted as Unit"
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, return_value_after_assignment)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(): Int =
            var _count = 5;
            _count = _count + 16 /* Assignment is interpreted as Unit */
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, return_value_after_while)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(): Double =
            var _count = 5.5;
            while _count < 100 do
                _count = _count + 16
                _count // Does not work as the return value
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, return_value_unit_discard_return)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(): Unit =
            return 5;
    )");

    EXPECT_EQ(exit_code(), 0);
}

// ===========================================================

TEST_F(analysis, if_return_valid_explicit_unit)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int): Unit =
            if i > 5 then
                5
            else
                i
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, if_return_invalid_implicit_unit)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int) =
            if i > 5 then
                5
            else
                i
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, if_return_valid_implicit_unit_not_return)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int) =
            if i > 5 then
                5
            else
                i
            5
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, if_return_invalid_explicit_type)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int): Int =
            if i > 5 then
                5
            else
                i
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, if_return_valid_explicit_type_all_branches_return)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int): Int =
            if i > 5 then
                return 5;
            else
                return i;
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, if_return_invalid_explicit_type_not_all_branches_return)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int): Int =
            if i > 5 then
                5
            else
                return i;
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, if_return_valid_implicit_all_branches_unit)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int) =
            if i > 5 then
                val x = i;
            else
                ()
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, if_return_valid_implicit_all_branches_unit_recursive)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int) =
            if i > 5 then
                if i > 5 then
                    val x = i;
                else
                    val y = i
            else
                ()
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, if_return_invalid_implicit_all_branches_unit_recursive)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int) =
            if i > 5 then
                if i > 5 then
                    val x = i;
                else
                    5
            else
                ()
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, if_return_valid_implicit_all_branches_unit_without_else)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int) =
            if i > 5 then
                val x = i;
    )");

    EXPECT_EQ(exit_code(), 0);
}

// ===========================================================

TEST_F(analysis, recursion_direct_valid_explicit)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int): Int =
            if i > 0 then
               something(i - 1)
            i
    )");

    EXPECT_EQ(exit_code(true), 0);
}

TEST_F(analysis, recursion_direct_invalid_implicit)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();
        def something(i: Int) =
            if i > 0 then
               something(i - 1)
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, recursion_indirect_valid_explicit)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(i: Int): Int =
            val x = other(i)
            x

        def other(i: Int): Int =
            something(i - 1)
    )");

    EXPECT_EQ(exit_code(true), 0);
}

TEST_F(analysis, recursion_indirect_valid_explicit_partial)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(i: Int) =
            other(i)

        def other(i: Int): Int =
            something(i - 1)
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, recursion_indirect_invalid_implicit_all)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(i: Int) =
            val x = other(i)
            x

        def other(i: Int) =
            something(i - 1)
    )");

    EXPECT_EQ(exit_code(), 6);
}

// ===========================================================

TEST_F(analysis, function_call_statement)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def squared(value: Double) = value * value

        def something(num: Double) =
            squared(squared(num))
            squared(5)
            squared(num)
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, function_call_statement_before_definition)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double) =
            squared(squared(num))
            squared(5)
            squared(num)

        def squared(value: Double) = value * value
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, function_call_var_initialization)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double) =
            var x = twice(twice(num))
            val y = twice(5)
            x

        def twice(value: Double) = 2 * value
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, function_call_assignment_value)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double) =
            var x = 5.5
            val y = 8
            x = add_one(add_one(y))
            x

        def add_one(value: Double) = (value + 1);
    )");

    EXPECT_EQ(exit_code(true), 0);
}

TEST_F(analysis, function_call_single_line_function)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            add_two(num);

        def add_two(value: Double) = add_one(add_one(value))
        def add_one(value: Double) = (value + 1);
    )");

    EXPECT_EQ(exit_code(true), 0);
}

TEST_F(analysis, function_call_return)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            return add(num, 5.0);

        def add(a: Double, b: Double) = a + b;
    )");

    EXPECT_EQ(exit_code(true), 0);
}

TEST_F(analysis, function_call_invalid_parameter_count)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            return add(num, 5.0, 99999999);

        def add(a: Double, b: Double) = a + b;
    )");

    EXPECT_EQ(exit_code(), 5);
}

TEST_F(analysis, function_call_undefined)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            return add(num, 2);
    )");

    EXPECT_EQ(exit_code(), 3);
}

// ===========================================================

TEST_F(analysis, if_statement_condition_boolean)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            if (num * 2 > num - 2) && num != 0 then
                return num;
            else
                return num + 2;
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, if_statement_condition_not_boolean)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            if 5 + 5 then
                return num;
            else
                return num + 2;
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, if_statement_condition_boolean_variable)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            var _condition: Boolean = (num * 2 > num - 2) && num != 0;
            if _condition then
                return num;
            else
                return num + 2;
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, if_statement_condition_not_boolean_variable)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            if num then
                return num;
            else
                return num + 2;
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, if_statement_branch_scopes)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            var x = 1;
            if num > 5.5 then
                var x = 2;
                return num + x;
            else
                var x = 3;
                return num + x;
            return num + x;
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, if_statement_invalid_subindent_else)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            if num > 5.5 then
                return num + 2;
              else
                return num + 3;
    )");

    EXPECT_EQ(exit_code(), 1);
}

TEST_F(analysis, if_statement_invalid_missing_indent_else)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            if num > 5.5 then
                return num + 2;
                else
                return num + 3;
    )");

    EXPECT_EQ(exit_code(), 2);
}

TEST_F(analysis, if_statement_invalid_dedent_else)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
              if num > 5.5 then
                return num + 2;
            else
                return num + 3;
    )");

    EXPECT_EQ(exit_code(), 1);
}

// ===========================================================

TEST_F(analysis, while_statement_condition_boolean)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            var i = 0;
            while (i < 5.2) || i + 25 != 12 do
                i = i + 2;
            i + 1.0
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, while_statement_condition_not_boolean)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            var i = 0;
            while 848 - 5.5 * i do
                i = i + 2;
            i + 1.0
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, while_statement_condition_boolean_variable)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            var i = 0;
            var _condition = (i < 5.2) || i + 25 != 12
            while _condition do
                i = i + 2;
                _condition = (i < 5.2) || i + 25 != 12
            i + 1.0
    )");

    EXPECT_EQ(exit_code(), 0);
}

TEST_F(analysis, while_statement_condition_not_boolean_variable)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            val i = 0;
            var _condition = 848 - 5.5 * i;
            while _condition do
                i = i + 2;
            i + 1.0
    )");

    EXPECT_EQ(exit_code(), 6);
}

TEST_F(analysis, while_statement_branch_scopes)
{
    load_code(R"(
        import ifj26.*
        @main def nothing(): Unit = ();

        def something(num: Double): Double =
            var i = 0;
            while i < 10 do
                i = i + 2
                var i = 0;
                i = i + 3;
            i + 1.0
    )");

    EXPECT_EQ(exit_code(), 0);
}