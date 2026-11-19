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
    expect_code = 0;
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
    expect_code = 0;
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
    expect_code = 0;
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
    expect_code = 0;
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
    expect_code = 0;
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
    expect_code = 0;
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
    expect_code = 0;
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
    expect_code = 0;
}
