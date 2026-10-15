/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    scanner_tests.cpp
 * @brief   Automated unit tests for the scanner module
 * @author  Filip Hajdusek (xhajduf00)
 */

#include <gtest/gtest.h>

extern "C" {
#include "scanner.h"
}

// =====================================================

class scanner : public ::testing::Test {
protected:
    // Temporary input file for stdin emulation
    FILE* input = nullptr;

    /** @brief Setups an input stream for each scanner test */
    void SetUp() override
    {
        input = tmpfile();
        ASSERT_NE(input, nullptr);

        ScannerContext scanner_context;
        init_scanner(&scanner_context);
    }

    /** @brief Closes the input stream after each scanner test */
    void TearDown() override
    {
        if (input != nullptr) {
            fclose(input);
        }
    }

    /**
     * @brief Writes a given string to the scanner input stream
     * @param string the input string to be written at the start of the stream
     */
    void set_scanner_input(const char* string) const
    {
        ASSERT_NE(input, nullptr);
        ASSERT_GE(fputs(string, input), 0);
        rewind(input);
    }

    Token scan(const char* s) const
    {
        set_scanner_input(s);
        return get_token(input);
    }
};

// =====================================================

TEST_F(scanner, err_invalid_char)
{
    ASSERT_EQ(Token::ERROR, scan("\0").type);
}

// =====================================================
// Integer tests

TEST_F(scanner, int_simple)
{
    const Token t = scan("123");
    ASSERT_EQ(t.type, Token::INTEGER);
    EXPECT_EQ(t.attribute_int, 123);
}

TEST_F(scanner, int_underscores)
{
    const Token t = scan("1_23_455");
    ASSERT_EQ(t.type, Token::INTEGER);
    EXPECT_EQ(t.attribute_int, 123455);
}

TEST_F(scanner, int_zero)
{
    const Token t = scan("0");
    ASSERT_EQ(t.type, Token::INTEGER);
    EXPECT_EQ(t.attribute_int, 0);
}

// Integer error handling tests

TEST_F(scanner, int_err_leading_zero)
{
    const Token token = scan("0123");
    ASSERT_EQ(Token::ERROR, token.type);
}

// =====================================================