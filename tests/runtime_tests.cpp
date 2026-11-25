/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    runtime_tests.cpp
 * @brief   Automated complex tests for lexical, syntax, semantic analysis,
 *          code generation and interpretation
 * @author  Filip Hajdusek (xhajduf00)
 */

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>

using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;

struct CompilerResult {
    string flags = "";
    int code = -1;
    string errors;
};

struct InterpreterResult {
    int code = -1;
    string errors;
    string output;
};

const string COMPILER = "../../ifj26compiler";
const string INTERPRETER = "ic26int-linux-x86_64";

class runtime : public ::testing::Test {
protected:
    CompilerResult compiler;
    InterpreterResult interpreter;

    string source;
    string input;
    string output;

    bool ast = false; // Add --ast and other flags to the compiler?
    int expect_code = 0;

    bool run_compiler(const string& ifj26, const string& ifj26code, const string& err)
    {
        ofstream(tmp(ifj26)) << source;

        string cmd = COMPILER + (' ') + compiler.flags;
        cmd += " < " + tmp(ifj26);
        cmd += " > " + tmp(ifj26code);
        cmd += " 2> " + tmp(err);

        compiler.code = shell(cmd);
        compiler.errors = read_file(tmp(err));

        return compiler.code == 0;
    }

    bool run_interpreter(const string& ifj26code, const string& in, const string& out, const string& err)
    {
        ofstream(tmp(in)) << input;

        string cmd = INTERPRETER;
        cmd += " " + tmp(ifj26code);
        cmd += " < " + tmp(in);
        cmd += " > " + tmp(out);
        cmd += " 2> " + tmp(err);

        interpreter.code = shell(cmd);
        interpreter.errors = read_file(tmp(err));
        interpreter.output = read_file(tmp(out));

        return interpreter.code == 0;
    }

    int exit_code()
    {
        if (!run_compiler("n.ifj26", "n.ifj26code", "compiler.err"))
            return compiler.code;

        if (!run_interpreter("n.ifj26code", "n.in", "n.out", "interpreter.err"))
            return interpreter.code;

        return 0;
    }

    // ==================================================

    void SetUp() override
    {
        std::filesystem::create_directories(tmp());
    }

    void TearDown() override
    {
        // Show the the line of the actual test as well
        const auto* info = testing::UnitTest::GetInstance()->current_test_info();
        testing::ScopedTrace trace(info->file(), info->line(), "Failed test");

        EXPECT_EQ(exit_code(), expect_code);
        EXPECT_EQ(interpreter.output, output);

        if (HasFailure() || compiler.flags != "")
            std::cout << compiler.errors << interpreter.errors;
    }

    // ==================================================

private:
    static string tmp(const string& s = "")
    {
        return "./tmp/" + s;
    }

    static string read_file(const string& path)
    {
        ifstream file(path);
        stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    static int shell(const string& command)
    {
        const int status = std::system(command.c_str());
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
};

class base_lex : public runtime { };
class base_struct : public runtime { };
class base_vars : public runtime { };
class base_funcs : public runtime { };
class base_expr : public runtime { };
class boolthen : public runtime { };
class funexp : public runtime { };
class valmem : public runtime { };
class extkw : public runtime { };
class interpolateds : public runtime { };
class paramlists : public runtime { };
class combo : public runtime { };

// clang-format off
// (raw strings contain indentation-sensitive IFJ26 code - the formatter must not touch them)

// ========================================================
// 1. ZAKLADNI FUNKCIONALITA - LEXIKALNI ANALYZA (BASE_LEX)
// ========================================================

TEST_F(base_lex, numeric_underscores_and_double_formats)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val i = 1_000_000
            val z = 0
            val d1 = .5
            val d2 = 2.5e2
            val d3 = 1_2.3_4E-1D
            val d4 = 5e+1d
            val d5 = 1e0
            print(toStr(i) + "|" + toStr(z) + "|" + toStr(d1) + "|" + toStr(d2) + "|" + toStr(d3) + "|" + toStr(d4) + "|" + toStr(d5))
    )IFJ";

    output = "1000000|0|0.50|250.00|1.23|50.00|1.00";
}

TEST_F(base_lex, case_sensitivity_identifiers)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val value = 1
            val Value = 2
            val VALUE = 3
            val _val_1 = 4
            val __x = 5
            print(toStr(value + Value + VALUE + _val_1 + __x))
    )IFJ";

    output = "15";
}

TEST_F(base_lex, string_escapes_and_multiline_literal)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val s1 = "a\\b\"c\nd\te\rf"
            val s2 = """line1\n
"quoted" ""inner""
line3"""
            print(s1 + "|" + s2)
    )IFJ";

    output = "a\\b\"c\nd\te\rf|line1\\n\n\"quoted\" \"\"inner\"\"\nline3";
}

TEST_F(base_lex, nested_block_comments_and_semicolons)
{
    /* language=.ifj26 */
    source = R"IFJ(
        /* komentare pred prologem
           /* vcetne vnoreneho /* i treti urovne */ */
        */
        import ifj26.*

        @main def main(): Unit =
            var a = 1; var b = /* inline */ 2; a = a/*x*/+/*y*/b;
            /* vice
               radkovy komentar */
            print(a)
    )IFJ";

    output = "3";
}

TEST_F(base_lex, multiline_parentheses_and_trailing_commas)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def sumThree(
            a: Int,
            b: Int,
            c: Int,
            ): Int =
            val total = (
                a +
                    b +
                    c
                )
            return total

        @main def main(): Unit =
            val res = sumThree(
                10,
                20,
                30,
                )
            print(res)
    )IFJ";

    output = "60";
}

TEST_F(base_lex, blank_lines_with_spaces_and_eof_line_comment)
{
    /* language=.ifj26 */
    source = "import ifj26.*\n@main def main(): Unit =\n    val x = 7\n  \n      \n    print(x)\n// komentar na konci bez newline";

    output = "7";
}

TEST_F(base_lex, err_leading_zero_in_int)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 012
            print(x)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_int_literal_overflow)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 2147483648
            print(x)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_trailing_underscore_in_number)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 100_
            print(x)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_double_underscore_in_number)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 1__000
            print(x)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_underscore_next_to_dot_in_double)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 1_.5
            print(x)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_trailing_dot_without_fractional_digits)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 1.
            print(x)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_hex_literal_not_supported)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 0x10
            print(x)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_long_suffix_not_supported)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 10L
            print(x)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_unsupported_float_suffix_f)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 1.5f
            print(x)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_single_underscore_identifier)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val _ = 5
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_tab_indentation)
{
    /* language=.ifj26 */
    source = "import ifj26.*\n@main def main(): Unit =\n\tprint(1)\n";

    expect_code = 1;
}

TEST_F(base_lex, err_unclosed_nested_block_comment)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            /* vnejsi /* vnitrni */
            print(1)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_unclosed_string_literal)
{
    /* language=.ifj26 */
    source = "import ifj26.*\n@main def main(): Unit =\n    val s = \"neukonceny\n";

    expect_code = 1;
}

TEST_F(base_lex, err_invalid_escape_in_string)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val s = "ahoj\asvete"
            print(s)
    )IFJ";

    expect_code = 1;
}

TEST_F(base_lex, err_tokens_after_multiline_block_comment)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            /* prvni radek
               druhy radek */ val x = 1
            print(x)
    )IFJ";

    expect_code = 2;
}

TEST_F(base_lex, err_trailing_comma_without_newline)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def f(a: Int, b: Int): Int = a + b

        @main def main(): Unit =
            val x = f(1, 2, )
            print(x)
    )IFJ";

    expect_code = 2;
}

TEST_F(base_lex, err_logical_line_starting_with_operator)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 1
                + 2
            print(x)
    )IFJ";

    expect_code = 2;
}

TEST_F(base_lex, err_underindented_multiline_paren_continuation)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = (
            1 + 2
                )
            print(x)
    )IFJ";

    expect_code = 2;
}

TEST_F(base_lex, err_closing_paren_same_indent_as_open)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = (
                1 + 2
            )
            print(x)
    )IFJ";

    expect_code = 2;
}

TEST_F(base_lex, err_inconsistent_dedent_level)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val a = 1
          val b = 2
            print(a)
    )IFJ";

    expect_code = 2;
}

TEST_F(base_lex, err_keyword_as_variable_name)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val while = 1
            print(while)
    )IFJ";

    expect_code = 2;
}

// ========================================================
// 2. ZAKLADNI FUNKCIONALITA - STRUKTURA PROGRAMU (BASE_STRUCT)
// ========================================================

TEST_F(base_struct, custom_main_name_and_standalone_annotation)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main
        def customEntryPoint(): Unit =
            print("ok")
    )IFJ";

    output = "ok";
}

TEST_F(base_struct, inferred_unit_on_main_and_helper_named_main)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def main(x: Int): Int = x + 1

        @main def start() =
            val r = main(4)
            print(r)
    )IFJ";

    output = "5";
}

TEST_F(base_struct, err_missing_prolog)
{
    /* language=.ifj26 */
    source = R"IFJ(
        @main def main(): Unit =
            print("no prolog")
    )IFJ";

    expect_code = 2;
}

TEST_F(base_struct, err_duplicate_prolog)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*
        import ifj26.*

        @main def main(): Unit =
            print("dup")
    )IFJ";

    expect_code = 2;
}

TEST_F(base_struct, err_top_level_variable_not_allowed)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        val globalVar = 10

        @main def main(): Unit =
            print(globalVar)
    )IFJ";

    expect_code = 2;
}

TEST_F(base_struct, err_missing_main)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def helper(): Unit =
            print("hi")
    )IFJ";

    expect_code = 3;
}

TEST_F(base_struct, err_multiple_mains)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def first(): Unit =
            ()

        @main def second(): Unit =
            ()
    )IFJ";

    expect_code = 4;
}

TEST_F(base_struct, err_main_with_parameter)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(x: Int): Unit =
            print(x)
    )IFJ";

    expect_code = 5;
}

TEST_F(base_struct, err_main_non_unit_return_type)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Int =
            return 0
    )IFJ";

    expect_code = 6;
}

TEST_F(base_struct, err_empty_function_body)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
        def other(): Unit =
            ()
    )IFJ";

    expect_code = 2;
}

// ========================================================
// 3. ZAKLADNI FUNKCIONALITA - PROMENNE A SCOPING (BASE_VARS)
// ========================================================

TEST_F(base_vars, implicit_int_to_double_and_unit_variable)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            var d: Double = 5
            d = 12
            val u: Unit = ()
            print(toStr(d) + "|")
            print(u)
    )IFJ";

    output = "12.00|()";
}

TEST_F(base_vars, nested_shadowing_with_self_reference)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            var x = 10
            if x == 10 then
                val x = x + 5
                print(x)
                print("|")
                var i = 0
                while i < 1 do
                    val x = x * 2
                    print(x)
                    print("|")
                    i = i + 1
                print(x)
                print("|")
            else
                ()
            print(x)
    )IFJ";

    output = "15|30|15|10";
}

TEST_F(base_vars, sibling_blocks_and_parameter_shadowing_in_subblock)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def testScope(p: Int): Int =
            if p > 0 then
                val p = "shadowed"
                print(p)
            else
                val p = 999
                print(p)
            print("|")
            var k = 0
            while k < 1 do
                val temp = p + 1
                print(temp)
                k = k + 1
            while k < 2 do
                val temp = p + 2
                print("|")
                print(temp)
                k = k + 1
            return p

        @main def main(): Unit =
            val res = testScope(5)
            print("|")
            print(res)
    )IFJ";

    output = "shadowed|6|7|5";
}

TEST_F(base_vars, err_double_to_int_conversion)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x: Int = 3.14
            print(x)
    )IFJ";

    expect_code = 6;
}

TEST_F(base_vars, err_assign_wrong_type_to_var)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            var x = 10
            x = "ten"
    )IFJ";

    expect_code = 6;
}

TEST_F(base_vars, err_assign_to_val)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 1
            x = 2
    )IFJ";

    expect_code = 7;
}

TEST_F(base_vars, err_assign_to_parameter)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def f(p: Int): Unit =
            p = 5

        @main def main(): Unit =
            f(1)
    )IFJ";

    expect_code = 7;
}

TEST_F(base_vars, err_redefine_in_same_block)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 1
            var x = 2
            print(x)
    )IFJ";

    expect_code = 4;
}

TEST_F(base_vars, err_redefine_parameter_in_top_block)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def f(x: Int): Int =
            val x = 10
            return x

        @main def main(): Unit =
            val r = f(1)
            print(r)
    )IFJ";

    expect_code = 4;
}

TEST_F(base_vars, err_use_variable_out_of_scope)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            if 1 < 2 then
                val secret = 42
            else
                ()
            print(secret)
    )IFJ";

    expect_code = 3;
}

TEST_F(base_vars, err_self_reference_in_first_declaration)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = x + 1
            print(x)
    )IFJ";

    expect_code = 3;
}

// ========================================================
// 4. ZAKLADNI FUNKCIONALITA - FUNKCE A NAVRATY (BASE_FUNCS)
// ========================================================

TEST_F(base_funcs, forward_call_and_mutual_recursion)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val a = isEven(6)
            val b = isOdd(6)
            print(toStr(a) + "|" + toStr(b))

        def isEven(n: Int): Int =
            if n == 0 then
                return 1
            else
                return isOdd(n - 1)

        def isOdd(n: Int): Int =
            if n == 0 then
                return 0
            else
                return isEven(n - 1)
    )IFJ";

    output = "1|0";
}

TEST_F(base_funcs, deep_recursion_preserves_local_frames)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def sumRec(n: Int): Int =
            val keep = n * 2
            if n <= 0 then
                return 0
            else
                val sub = sumRec(n - 1)
                return sub + keep / 2

        @main def main(): Unit =
            val r = sumRec(50)
            print(r)
    )IFJ";

    output = "1275";
}

TEST_F(base_funcs, unit_function_discards_expression_and_return_value)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def sideEffect(): Int =
            print("A")
            return 99

        def f1(): Unit =
            sideEffect()

        def f2(): Unit =
            return sideEffect()

        @main def main(): Unit =
            val u1 = f1()
            val u2 = f2()
            print("|")
            print(u1)
            print("|")
            print(u2)
    )IFJ";

    output = "AA|()|()";
}

TEST_F(base_funcs, inferred_unit_from_assignment_while_and_if)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def endsWithAssign() =
            var x = 1
            x = 2

        def endsWithWhile() =
            var i = 0
            while i < 2 do
                i = i + 1

        def endsWithUnitIf(flag: Int) =
            if flag == 1 then
                print("T")
            else
                ()

        @main def main(): Unit =
            val a = endsWithAssign()
            val b = endsWithWhile()
            val c = endsWithUnitIf(1)
            print("|")
            print(a)
            print(b)
            print(c)
    )IFJ";

    output = "T|()()()";
}

TEST_F(base_funcs, single_line_functions_and_implicit_double_return)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def addOne(x: Int) = x + 1
        def asDouble(x: Int): Double = x

        @main def main(): Unit =
            val a = addOne(9)
            val b = asDouble(a)
            print(toStr(b))
    )IFJ";

    output = "10.00";
}

TEST_F(base_funcs, err_recursion_without_explicit_return_type)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def loop(n: Int) =
            if n == 0 then
                return 0
            else
                return loop(n - 1)

        @main def main(): Unit =
            val x = loop(3)
            print(x)
    )IFJ";

    expect_code = 6;
}

TEST_F(base_funcs, err_return_without_explicit_return_type)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def f(x: Int) =
            return x + 1

        @main def main(): Unit =
            val y = f(1)
            print(y)
    )IFJ";

    expect_code = 6;
}

TEST_F(base_funcs, err_bare_return_in_non_unit_function)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def f(): Int =
            return

        @main def main(): Unit =
            val x = f()
            print(x)
    )IFJ";

    expect_code = 6;
}

TEST_F(base_funcs, err_non_unit_function_ending_with_if_without_return)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def f(x: Int): Int =
            if x > 0 then
                return 1
            else
                2

        @main def main(): Unit =
            val y = f(1)
            print(y)
    )IFJ";

    expect_code = 6;
}

TEST_F(base_funcs, err_unreachable_type_mismatch_after_return)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def f(): Int =
            return 1
            "bad"

        @main def main(): Unit =
            val x = f()
            print(x)
    )IFJ";

    expect_code = 6;
}

TEST_F(base_funcs, err_function_redefinition)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def dup(x: Int): Int = x
        def dup(x: Double): Double = x

        @main def main(): Unit =
            ()
    )IFJ";

    expect_code = 4;
}

TEST_F(base_funcs, err_call_undefined_function)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            doesNotExist(1)
    )IFJ";

    expect_code = 3;
}

TEST_F(base_funcs, err_wrong_argument_count)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def pair(a: Int, b: Int): Int = a + b

        @main def main(): Unit =
            val x = pair(1)
            print(x)
    )IFJ";

    expect_code = 5;
}

// ========================================================
// 5. ZAKLADNI FUNKCIONALITA - VYRAZY A VESTAVENE (BASE_EXPR)
// ========================================================

TEST_F(base_expr, precedence_associativity_and_truncating_div)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val a = 10 - 3 - 2 + 4 * 5 / 2
            val b = -7 / 2
            val c = 7 / -2
            val d = -7.0 / 2
            val e = --5
            print(toStr(a) + "|" + toStr(b) + "|" + toStr(c) + "|" + toStr(d) + "|" + toStr(e))
    )IFJ";

    output = "15|-3|-3|-3.50|5";
}

TEST_F(base_expr, int32_overflow_wrapping_semantics)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val maxInt = 2147483647
            val minInt = -2147483647 - 1
            val ovfAdd = maxInt + 1
            val ovfSub = minInt - 1
            val ovfNeg = -minInt
            val ovfMul = 100000 * 100000
            print(toStr(ovfAdd) + "|" + toStr(ovfSub) + "|" + toStr(ovfNeg) + "|" + toStr(ovfMul))
    )IFJ";

    output = "-2147483648|2147483647|-2147483648|1410065408";
}

TEST_F(base_expr, string_concat_and_comparisons)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val s = "ab" + "cd"
            if "abc" < "abd" then
                print("1")
            else
                print("0")
            if 5 == 5.0 then
                print("1")
            else
                print("0")
            if s != "abcd" then
                print("0")
            else
                print("1")
            if () == () then
                print("1")
            else
                print("0")
    )IFJ";

    output = "1111";
}

TEST_F(base_expr, double_hex_print_vs_tostr)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val d = 3.0
            print(d)
            print("|")
            print(toStr(d))
    )IFJ";

    output = "0x1.8p+1|3.00";
}

TEST_F(base_expr, builtin_io_and_string_conversions)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val i1 = readInt()
            val i2 = readInt()
            val d1 = readDouble()
            val l1 = readLine()
            val t1 = toInt(3.99)
            val t2 = toInt(-3.99)
            val o1 = ord("ABC", 1)
            val o2 = ord("", 0)
            val o3 = ord("ABC", 3)
            val c1 = chr(66)
            print(toStr(i1) + "|" + toStr(i2) + "|" + toStr(d1) + "|" + l1)
            print(toStr(t1) + "|" + toStr(t2) + "|" + toStr(o1) + "|" + toStr(o2) + "|" + toStr(o3) + "|" + c1)
    )IFJ";

    input = "42\nnot_an_int\n2.75\nhello\n";
    output = "42|0|2.75|hello\n3|-3|66|-1|-1|B";
}

TEST_F(base_expr, substr_and_chr_boundary_cases)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val s = "abcdef"
            val a = subStr(s, 0, 6)
            val b = subStr(s, 2, 5)
            val c = subStr(s, 6, 6)
            val d = subStr("", 0, 0)
            val l1 = strLength(chr(0))
            val l2 = strLength(chr(127))
            print(a + "|" + b + "|[" + c + "]|[" + d + "]|" + toStr(l1) + "|" + toStr(l2))
    )IFJ";

    output = "abcdef|cde|[]|[]|1|1";
}

TEST_F(base_expr, runtime_substr_negative_start)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val s = subStr("abc", -1, 2)
            print(s)
    )IFJ";

    expect_code = 40;
}

TEST_F(base_expr, runtime_substr_start_greater_than_end)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val s = subStr("abc", 2, 1)
            print(s)
    )IFJ";

    expect_code = 40;
}

TEST_F(base_expr, runtime_substr_end_exceeds_length)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val s = subStr("abc", 1, 4)
            print(s)
    )IFJ";

    expect_code = 40;
}

TEST_F(base_expr, runtime_chr_out_of_range_high)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val s = chr(128)
            print(s)
    )IFJ";

    expect_code = 58;
}

TEST_F(base_expr, runtime_chr_out_of_range_negative)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val s = chr(-1)
            print(s)
    )IFJ";

    expect_code = 58;
}

TEST_F(base_expr, err_string_plus_int)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val s = "x" + 1
            print(s)
    )IFJ";

    expect_code = 6;
}

TEST_F(base_expr, err_chained_comparison)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            if 1 < 2 < 3 then
                print(1)
            else
                print(0)
    )IFJ";

    expect_code = 6;
}

TEST_F(base_expr, err_compare_incompatible_types)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            if 1 == "1" then
                print(1)
            else
                print(0)
    )IFJ";

    expect_code = 6;
}

TEST_F(base_expr, runtime_int_div_by_zero)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val z = 0
            val x = 10 / z
            print(x)
    )IFJ";

    expect_code = 57;
}

TEST_F(base_expr, runtime_double_div_by_zero)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val z = 0.0
            val x = 10.0 / z
            print(x)
    )IFJ";

    expect_code = 57;
}

// ========================================================
// 6. ROZSIRENI FUNEXP - FUNKCE VE VYRAZECH
// ========================================================

TEST_F(funexp, nested_calls_and_left_to_right_eval)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def trace(tag: String, v: Int): Int =
            print(tag)
            return v

        def combine(a: Int, b: Int, c: Int): Int =
            return a * 100 + b * 10 + c

        @main def main(): Unit =
            val r = combine(trace("A", 1) + trace("B", 2), trace("C", 4), trace("D", 5))
            print("|")
            print(r)
    )IFJ";

    output = "ABCD|345";
}

TEST_F(funexp, implicit_conversion_in_nested_call_expr)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def half(x: Double): Double = x / 2.0
        def three(): Int = 3

        @main def main(): Unit =
            val res = half(three() + 2) * 3
            print(toStr(res))
    )IFJ";

    output = "7.50";
}

TEST_F(funexp, builtins_and_user_calls_in_conditions_and_loops)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def next(x: Int): Int = x + 1

        @main def main(): Unit =
            var i = 0
            while next(i) <= strLength(subStr("abcdef", 1, 5)) do
                print(toStr(i * 10 + next(i)) + ";")
                i = next(i)
    )IFJ";

    output = "1;12;23;34;";
}

TEST_F(funexp, err_unit_call_inside_arithmetic_expr)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def noop(): Unit = ()

        @main def main(): Unit =
            val x = 1 + noop()
            print(x)
    )IFJ";

    expect_code = 6;
}

TEST_F(funexp, err_wrong_argument_type_in_nested_call)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def inc(x: Int): Int = x + 1
        def text(): String = "no"

        @main def main(): Unit =
            print(inc(text()))
    )IFJ";

    expect_code = 5;
}

// ========================================================
// 7. ROZSIRENI BOOLTHEN - PRAVDIVOSTNI HODNOTY
// ========================================================

TEST_F(boolthen, operator_precedence_negation_and_or)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val a = true || false && false
            val b = !false && (2 + 3 == 5)
            val c = !(1 > 2) == true
            val d = false != false
            val e = !!true && !!!false
            print(toStr(a) + "|" + toStr(b) + "|" + toStr(c) + "|" + toStr(d) + "|" + toStr(e))
    )IFJ";

    output = "true|true|true|false|true";
}

TEST_F(boolthen, short_circuit_evaluation_skips_rhs)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def bomb(): Boolean =
            print("BOOM")
            val z = 0
            return (1 / z) == 0

        @main def main(): Unit =
            val a = false && bomb()
            val b = true || bomb()
            print(toStr(a) + "|" + toStr(b))
    )IFJ";

    output = "false|true";
}

TEST_F(boolthen, read_boolean_and_print)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val b1 = readBoolean()
            val b2 = readBoolean()
            print(b1)
            print("|")
            print(b2)
    )IFJ";

    input = "true\nfalse\n";
    output = "true|false";
}

TEST_F(boolthen, return_boolean_and_use_in_while)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def keepGoing(i: Int, limit: Int): Boolean =
            return i < limit && !(i == 2)

        @main def main(): Unit =
            var i = 0
            while keepGoing(i, 5) do
                print(i)
                i = i + 1
    )IFJ";

    output = "01";
}

TEST_F(boolthen, shortened_if_and_else_if_chains)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def classify(n: Int): String =
            if n < 0 then
                return "neg"
            else if n == 0 then
                return "zero"
            else if n < 10 then
                return "small"
            else
                return "big"

        @main def main(): Unit =
            var flag = true
            if !flag then
                print("NO")
            if flag then
                print("YES|")
            print(classify(-5) + "|" + classify(0) + "|" + classify(4) + "|" + classify(42))
    )IFJ";

    output = "YES|neg|zero|small|big";
}

TEST_F(boolthen, err_non_boolean_operand_in_and)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val b = true && 1
            print(b)
    )IFJ";

    expect_code = 6;
}

TEST_F(boolthen, err_negate_int)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val b = !5
            print(b)
    )IFJ";

    expect_code = 6;
}

TEST_F(boolthen, err_relational_on_booleans)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            if false < true then
                print(1)
    )IFJ";

    expect_code = 6;
}

TEST_F(boolthen, err_non_boolean_if_condition)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            if 1 then
                print(1)
    )IFJ";

    expect_code = 6;
}

TEST_F(boolthen, err_misaligned_else_if)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 1
            if x < 0 then
                print("neg")
              else if x == 0 then
                print("zero")
    )IFJ";

    expect_code = 2;
}

// ========================================================
// 8. ROZSIRENI VALMEM - CLENY HODNOT
// ========================================================

TEST_F(valmem, to_string_on_all_supported_types)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val i = 42
            val d = 1.2345
            val s = "hi"
            val u = ()
            print(i.toString + "|" + d.toString + "|" + s.toString + "|" + u.toString)
    )IFJ";

    output = "42|1.23|hi|()";
}

TEST_F(valmem, double_floor_and_to_int_positive_and_negative)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val a = (3.9).floor
            val b = (-2.1).floor
            val c = (-2.0).floor
            val d = (3.9).toInt
            val e = (-2.9).toInt
            val f = (0.0).floor
            print(a.toString + "|" + b.toString + "|" + c.toString + "|" + d.toString + "|" + e.toString + "|" + f.toString)
    )IFJ";

    output = "3.00|-3.00|-2.00|3|-2|0.00";
}

TEST_F(valmem, string_length_and_chaining)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val emptyLen = "".length
            val chained = (7.85).floor.toInt.toString
            val sLen = ("abc" + "defg").length.toString
            val deep = (-4.2).floor.toInt.toString.length.toString
            print(emptyLen.toString + "|" + chained + "|" + sLen + "|" + deep)
    )IFJ";

    output = "0|7|7|2";
}

TEST_F(valmem, err_length_with_parentheses)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            print("abc".length())
    )IFJ";

    expect_code = 5;
}

TEST_F(valmem, err_floor_on_int)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 5
            print(x.floor)
    )IFJ";

    expect_code = 3;
}

TEST_F(valmem, err_to_int_on_string)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            print("123".toInt)
    )IFJ";

    expect_code = 3;
}

TEST_F(valmem, err_unknown_member)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            print("abc".upper)
    )IFJ";

    expect_code = 3;
}

// ========================================================
// 9. ROZSIRENI INTERPOLATEDS - INTERPOLOVANE RETEZCE
// ========================================================

TEST_F(interpolateds, literal_dollar_and_all_types)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val name = "IFJ"
            val n = 26
            val price = 1.2345
            val u = ()
            print(s"$name${n + 1} costs $$$price | $u")
    )IFJ";

    output = "IFJ27 costs $1.23 | ()";
}

TEST_F(interpolateds, adjacent_vars_punctuation_and_nested_interpolation)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val a = "X"
            val b = 9
            val nested = s"[$a$b=${s"${b * 2}"}]"
            print(nested)
    )IFJ";

    output = "[X9=18]";
}

TEST_F(interpolateds, nested_string_and_braces_inside_placeholder)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val x = 3
            print(s"${"val=" + toStr((x + 1) * 2)}")
    )IFJ";

    output = "val=8";
}

TEST_F(interpolateds, multiline_literal_quotes_and_escapes_uninterpreted)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val who = "World"
            print(s"""Hello "$who",
path C:\temp\n$$end""")
    )IFJ";

    output = "Hello \"World\",\npath C:\\temp\\n$end";
}

TEST_F(interpolateds, empty_interpolated_string)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val e = s""
            print("[" + e + "]")
    )IFJ";

    output = "[]";
}

TEST_F(interpolateds, err_unclosed_braces)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            print(s"${1 + 2")
    )IFJ";

    expect_code = 1;
}

TEST_F(interpolateds, err_invalid_dollar_identifier)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            print(s"$123")
    )IFJ";

    expect_code = 1;
}

TEST_F(interpolateds, err_undefined_variable_in_interpolation)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            print(s"$missingVar")
    )IFJ";

    expect_code = 3;
}

// ========================================================
// 10. ROZSIRENI EXTKW - UZIVATELSKE ROZSIRUJICI METODY
// ========================================================

TEST_F(extkw, int_extension_methods_zero_empty_and_multi_args)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (v: Int)
            def twice: Int = v * 2
            def tag(): String = "[" + toStr(v) + "]"
            def clamp(lo: Int, hi: Int): Int =
                if v < lo then
                    return lo
                else
                    if v > hi then
                        return hi
                    else
                        return v

        @main def main(): Unit =
            val x = 6
            print(toStr(x.twice) + "|" + x.tag() + "|" + toStr(x.clamp(0, 4)))
    )IFJ";

    output = "12|[6]|4";
}

TEST_F(extkw, extension_defined_after_main_and_same_name_different_types)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        @main def main(): Unit =
            val i = 5
            val s = "ab"
            val d = 2.5
            print(i.dup + "|" + s.dup + "|" + d.dup)

        extension (x: Int)
            def dup: String = toStr(x) + toStr(x)

        extension (x: String)
            def dup: String = x + x

        extension (x: Double)
            def dup: String = toStr(x) + "!"
    )IFJ";

    output = "55|abab|2.50!";
}

TEST_F(extkw, multiple_extension_blocks_for_same_type_and_internal_call)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (n: Int)
            def inc: Int = n + 1

        extension (n: Int)
            def plusTwo: Int = n.inc.inc

        @main def main(): Unit =
            val v = 10
            print(v.plusTwo)
    )IFJ";

    output = "12";
}

TEST_F(extkw, extension_on_unit_and_expression_receiver_and_recursion)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (u: Unit)
            def label: String = "unit-val"

        extension (x: Int)
            def squared: Int = x * x
            def fact: Int =
                if x <= 1 then
                    return 1
                else
                    return x * (x - 1).fact

        @main def main(): Unit =
            val u = ()
            print(u.label + "|" + toStr((2 + 3).squared) + "|" + toStr(5.fact))
    )IFJ";

    output = "unit-val|25|120";
}

TEST_F(extkw, extension_implicit_int_to_double_arg)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (base: Double)
            def addScaled(x: Double, k: Int): Double = base + x * k

        @main def main(): Unit =
            val d = 1.5
            print(toStr(d.addScaled(2, 3)))
    )IFJ";

    output = "7.50";
}

TEST_F(extkw, err_duplicate_extension_method_for_same_type)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (a: Int)
            def same: Int = a

        extension (b: Int)
            def same(): Int = b + 1

        @main def main(): Unit =
            ()
    )IFJ";

    expect_code = 4;
}

TEST_F(extkw, err_call_parenless_extension_with_parens)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (x: Int)
            def twice: Int = x * 2

        @main def main(): Unit =
            val a = 3
            print(a.twice())
    )IFJ";

    expect_code = 5;
}

TEST_F(extkw, err_call_empty_paren_extension_without_parens)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (x: Int)
            def foo(): Int = x * 2

        @main def main(): Unit =
            val a = 3
            print(a.foo)
    )IFJ";

    expect_code = 5;
}

TEST_F(extkw, err_mutate_extension_receiver)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (x: Int)
            def bad(): Unit =
                x = 10

        @main def main(): Unit =
            val a = 1
            a.bad()
    )IFJ";

    expect_code = 7;
}

TEST_F(extkw, err_extension_on_wrong_receiver_type)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (s: String)
            def onlyStr: Int = strLength(s)

        @main def main(): Unit =
            val n = 42
            print(n.onlyStr)
    )IFJ";

    expect_code = 3;
}

// ========================================================
// 11. ROZSIRENI PARAMLISTS - VICE SEZNAMU PARAMETRU
// ========================================================

TEST_F(paramlists, parenless_function_reevaluates_each_time)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def nextVal: Int =
            print("!")
            return readInt()

        @main def main(): Unit =
            val a = nextVal + nextVal * 10
            print("|")
            print(a)
    )IFJ";

    input = "3\n4\n";
    output = "!!|43";
}

TEST_F(paramlists, multiple_param_lists_and_empty_lists)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def calc(a: Int)(b: Int, c: Int)()(d: Double): Double =
            return (a + b * c) + d

        def frontEmpty()(x: Int): Int = x + 1
        def trailingEmpty(x: Int)(): Int = x + 2

        @main def main(): Unit =
            val r1 = calc(2)(3, 4)()(1.5)
            val r2 = frontEmpty()(10)
            val r3 = trailingEmpty(10)()
            print(toStr(r1) + "|" + toStr(r2) + "|" + toStr(r3))
    )IFJ";

    output = "15.50|11|12";
}

TEST_F(paramlists, evaluation_order_across_parameter_lists)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def step(s: String, v: Int): Int =
            print(s)
            return v

        def f(a: Int)(b: Int)(c: Int): Int = a + b + c

        @main def main(): Unit =
            val res = f(step("1", 1))(step("2", 2))(step("3", 3))
            print("|")
            print(res)
    )IFJ";

    output = "123|6";
}

TEST_F(paramlists, recursive_multi_param_list_and_inferred_parenless)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def defaultBase = 2

        def pow(base: Int)(exp: Int): Int =
            if exp == 0 then
                return 1
            else
                return base * pow(base)(exp - 1)

        @main def main(): Unit =
            val r = pow(defaultBase)(6)
            print(r)
    )IFJ";

    output = "64";
}

TEST_F(paramlists, err_duplicate_param_name_across_lists)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def bad(x: Int)(y: String, x: Double): Int = 0

        @main def main(): Unit =
            ()
    )IFJ";

    expect_code = 4;
}

TEST_F(paramlists, err_call_parenless_function_with_parens)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def answer: Int = 42

        @main def main(): Unit =
            val x = answer()
            print(x)
    )IFJ";

    expect_code = 5;
}

TEST_F(paramlists, err_call_empty_paren_function_without_parens)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def answer(): Int = 42

        @main def main(): Unit =
            val x = answer
            print(x)
    )IFJ";

    expect_code = 5;
}

TEST_F(paramlists, err_omit_empty_parameter_list)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def offset()(v: Int): Int = v + 1

        @main def main(): Unit =
            val x = offset(5)
            print(x)
    )IFJ";

    expect_code = 5;
}

TEST_F(paramlists, err_wrong_type_in_second_param_list)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def add(a: Int)(b: Int): Int = a + b

        @main def main(): Unit =
            val x = add(1)("two")
            print(x)
    )IFJ";

    expect_code = 5;
}

// ========================================================
// 12. VZAJEMNA SOUHRA ROZSIRENI (COMBO)
// ========================================================

TEST_F(combo, extkw_with_paramlists)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (base: Int)
            def linear(k: Int)(q: Int): Int = base * k + q
            def wrap()(prefix: String)(suffix: String): String =
                prefix + toStr(base) + suffix

        @main def main(): Unit =
            val x = 4
            print(toStr(x.linear(3)(2)) + "|" + x.wrap()("<")(">"))
    )IFJ";

    output = "14|<4>";
}

TEST_F(combo, valmem_extkw_funexp_chaining)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (n: Int)
            def triple: Int = n * 3

        extension (s: String)
            def surround(ch: String): String = ch + s + ch

        @main def main(): Unit =
            val res = (4.9).floor.toInt.triple.toString.surround("*")
            print(res)
    )IFJ";

    output = "*12*";
}

TEST_F(combo, boolthen_extkw_paramlists_choice_and_short_circuit)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (cond: Boolean)
            def choose(whenTrue: String)(whenFalse: String): String =
                if cond then
                    return whenTrue
                else
                    return whenFalse

        extension (x: Int)
            def boom: Boolean =
                print("FAIL")
                return (x / 0) == 0

        @main def main(): Unit =
            val z = 0
            val sc1 = false && z.boom
            val sc2 = true || z.boom
            val a = (10 > 3 && !sc1 && sc2).choose("YES")("NO")
            val b = (10 < 3 || sc1).choose("YES")("NO")
            print(a + "|" + b)
    )IFJ";

    output = "YES|NO";
}

TEST_F(combo, boolthen_valmem_tostring_and_else_if)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def isLongEnough(s: String, minLen: Int): Boolean =
            return s.length >= minLen

        @main def main(): Unit =
            val a = "hello"
            val b = "hi"
            if isLongEnough(a, 5) && !isLongEnough(b, 5) then
                print(true.toString + "|" + false.toString)
            else if isLongEnough(b, 1) then
                print("alt")
    )IFJ";

    output = "true|false";
}

TEST_F(combo, interpolateds_with_all_extensions_together)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def factor: Int = 3

        def mul(a: Int)(b: Int): Int = a * b

        extension (s: String)
            def exclaim(n: Int): String =
                if n > 0 then
                    return s + "!"
                else
                    return s

        @main def main(): Unit =
            val word = "ifj"
            val score = 7.89
            val ok = word.length == factor && true
            val msg = s"word=${word.exclaim(mul(factor)(2))} len=${word.length.toString} score=${score.floor.toInt} ok=${ok.toString}"
            print(msg)
    )IFJ";

    output = "word=ifj! len=3 score=7 ok=true";
}

TEST_F(combo, complex_pipeline_extkw_paramlists_valmem_interpolateds_boolthen)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        def stepBonus: Int = 5

        def evalTrace(tag: String)(v: Int): Int =
            print(tag)
            return v

        extension (n: Int)
            def between(lo: Int)(hi: Int): Boolean = n >= lo && n <= hi
            def formatRange(lo: Int)(hi: Int): String =
                if n.between(lo)(hi) then
                    return s"in[${lo.toString}..${hi.toString}]:${n.toString}"
                else if n < lo then
                    return s"low(${n.toString})"
                else
                    return s"high(${n.toString})"

        @main def main(): Unit =
            val raw = (12.9).floor.toInt + stepBonus
            val computed = evalTrace("L")(raw) + evalTrace("R")(3)
            val s1 = computed.formatRange(10)(25)
            val s2 = (computed - 15).formatRange(10)(25)
            val s3 = (computed + 10).formatRange(10)(25)
            print(s"|$s1|$s2|$s3")
    )IFJ";

    output = "LR|in[10..25]:20|low(5)|high(30)";
}

TEST_F(combo, multiline_interpolated_with_extkw_paramlists_and_shadowing)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (s: String)
            def repeatSep(sep: String)(count: Int): String =
                var acc = ""
                var i = 0
                while i < count do
                    if i > 0 then
                        acc = acc + sep
                    acc = acc + s
                    i = i + 1
                return acc

        @main def main(): Unit =
            val item = "A"
            val out = s"""Header: "${item.repeatSep("-")(3)}"
Count=${item.repeatSep("-")(3).length}
Price=$$99"""
            print(out)
    )IFJ";

    output = "Header: \"A-A-A\"\nCount=5\nPrice=$99";
}

TEST_F(combo, err_extkw_paramlists_missing_second_list)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (x: Int)
            def addTwoLists(a: Int)(b: Int): Int = x + a + b

        @main def main(): Unit =
            val v = 10
            print(v.addTwoLists(1))
    )IFJ";

    expect_code = 5;
}

TEST_F(combo, err_interpolateds_calling_unit_extension_in_arithmetic)
{
    /* language=.ifj26 */
    source = R"IFJ(
        import ifj26.*

        extension (x: Int)
            def logVal(): Unit = print(x)

        @main def main(): Unit =
            val x = 5
            print(s"${x.logVal() + 1}")
    )IFJ";

    expect_code = 6;
}

// clang-format on
