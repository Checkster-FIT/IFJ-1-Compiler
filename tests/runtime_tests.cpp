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
const string INTERPRETER = "../../ic26int-linux-x86_64";

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

class boolthen : public runtime { };
class funexp : public runtime { };
class valmem : public runtime { };
class extkw : public runtime { };
class interpolateds : public runtime { };
class paramlists : public runtime { };

// ========================================================

TEST_F(runtime, squared)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            var in = readInt();
            print(squared(in));

        def squared(value: Int) = value * value

    )";

    input = "8";
    output = "64";
}

TEST_F(runtime, undefined)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            var in = readInt();
            print(squared(in));

    )";

    expect_code = 3;
}

// ========================================================

TEST_F(runtime, case_sensitivity)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            var a = readInt();
            var A = readInt();
            print(A);
            print("\n");
            print(a);
    )";

    input = "1\n2";
    output = "2\n1";
}

TEST_F(runtime, invalid_character)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            print(`x`);
    )";

    expect_code = 1;
}

TEST_F(runtime, invalid_escape_sequence)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            print("Test \h");
    )";

    expect_code = 1;
}

TEST_F(runtime, multiline_ignore_escape_sequence)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            print("""Hello \n world""");
    )";

    output = "Hello \\n world";
}

// ========================================================

TEST_F(runtime, statement_order)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            print("A");
            var x = 0
            print("B");
            print(x);
            x = x + 8
            print("C");
            print(x);
    )";

    output = "AB0C8";
}

TEST_F(runtime, variable_initalization)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            var x = "Test";
            val y = something(x);
            print(y);
            print(x);
            var z = y + 8 * 5;
            print(z);

        def something(s: String) = strLength(s);
    )";

    output = "4Test44";
}

// ========================================================

TEST_F(runtime, scope_functions)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            something();
            nothing();

        def something() =
            var x = 5;
            print(x);

        def nothing() =
            var x = 0;
            print(x);
    )";

    output = "50";
}

TEST_F(runtime, scope_conditional_block)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            var x = 0;
            print(x);

            if true then
                print(x);
                var x = 1;
                print(x);
            else
                print(x);
                var x = 2;
                print(x);

            print(x);
    )";

    output = "0010";
}

TEST_F(runtime, scope_cycle_block)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            var x = "-";
            print(x);

            var counter = 0;
            while counter < 3 do
                print(x);
                var x = "=";
                print(counter);
                print(x);
                counter = counter + 1;

            print(x);
    )";

    output = "--0=-1=-2=-";
}

// ========================================================

TEST_F(boolthen, print)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            print(true)
            print(" ")
            print(false)
            print(" ")
            print(!true)
            print(" ")
            print(!false)
            print(" ")
            print(true && false)
            print(" ")
            print(true || false)
    )";

    output = "true false false true false true";
}

TEST_F(boolthen, read)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            print(readBoolean())
            print(" ")
            print(readBoolean())
            print(" ")
            print(!readBoolean())
            print(" ")
            print(!readBoolean())
            print(" ")
            print(readBoolean() && readBoolean())
            print(" ")
            print(readBoolean() || readBoolean())
    )";

    input = "true\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\n";
    output = "true false false true false true";
}

// ========================================================

TEST_F(extkw, simple)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        extension (x: Int)
            def even(): Boolean =
                if (x / 2) == ((x + 1) / 2) then
                    return true
                else
                    return false

        @main def main(): Unit =
            print("5: ")
            print(5.even())
            print("\n")
            print("8: ")
            print(8.even())
            print("\n")
    )";

    output = "5: false\n8: true\n";
}

TEST_F(extkw, shared_identifier)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        extension (s: String)
            def size: Int = strLength(s);

        extension (i: Int)
            def size: Int = i;

        @main def main(): Unit =
            print("Adam: ")
            print("Adam".size)
            print("\n")
            print("8: ")
            print((8).size)
            print("\n")
    )";

    output = "Adam: 4\n8: 8\n";
}

// ========================================================

TEST_F(paramlists, no_group)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        def something: Boolean =
            print("something");
            return true;

        @main def main(): Unit =
            if something then
                print("!");
                something
    )";

    output = "something!something";
}

TEST_F(paramlists, group_count)
{
    /* language=.ifj26 */
    source = R"code(
        import ifj26.*

        def a: Unit = print("(0)");
        def b(): Unit = print("(1)");
        def c()(): Unit = print("(2)");
        def d()()(): Unit = print("(3)");

        @main def main(): Unit =
            d()()();
            b()
            a;
            c()();
    )code";

    output = "(3)(1)(0)(2)";
}

// ========================================================

TEST_F(interpolateds, variables)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            val a = true
            val b = 54
            val c = 3.14
            val d = ()
            val e = "???"

            print(s"$a + $b + $c + $d + $e")
    )";

    output = "true + 54 + 3.14 + () + ???";
}

TEST_F(interpolateds, expressions)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        def double(s: String) = s + s

        @main def main(): Unit =
            val a = true

            print(s"${a && false} = ${1 + 5} ${double("???")}")
    )";

    output = "false = 6 ??????";
}

// ========================================================

TEST_F(valmem, to_string)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        @main def main(): Unit =
            var s = ".toString:";
            s = s + " " + 5.toString
            s = s + " " + (3.14).toString
            s = s + " " + true.toString
            s = s + " " + ().toString
            s = s + " " + "str".toString
            print(s);
    )";

    output = ".toString: 5 3.14 true () str";
}

// ========================================================

TEST_F(runtime, new_line_in_parenthesis)
{
    /* language=.ifj26 */
    source = R"(
        import ifj26.*

        def foo(
            ) =
            ()

        def doo(
            a: Int,
            ) =
            ()

        def hoo(
            a: Int,
            b
            :
            Int,
            ) =
            ()

        @main def main(): Unit =
            foo(
                )

            doo(5)
            doo(
                (5
                    +
                    5),
                )
            doo(5
                )
            doo(5,

                )

            hoo(5,

                6,
                )
    )";
}