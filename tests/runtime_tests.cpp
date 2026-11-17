/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    runtime_tests.cpp
 * @brief   Automated complex tests for lexical, syntax and semantic analysis
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

    int expect_code = 0;

    bool run_compiler(const string& ifj26, const string& ifj26code, const string& err)
    {
        ofstream(tmp(ifj26)) << source;

        string cmd = COMPILER;
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
        EXPECT_EQ(exit_code(), expect_code);
        EXPECT_EQ(interpreter.output, output);

        if (HasFailure())
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

// ========================================================

TEST_F(runtime, squared)
{
    /* language=.ifj26 */
    source = R"(

        import ifj26.*

        @main def nothing(): Unit =
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

        @main def nothing(): Unit =
            var in = readInt();
            print(squared(in));

    )";

    expect_code = 3;
}
