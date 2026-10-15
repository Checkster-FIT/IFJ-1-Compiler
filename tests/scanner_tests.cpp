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
    ScannerContext scanner_context = { };

    /** @brief Setups an input stream for each scanner test */
    void SetUp() override
    {
        input = tmpfile();
        ASSERT_NE(input, nullptr);

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
     * @param s the input string to be written at the start of the stream
     */
    void scanner_input(const char* s) const
    {
        ASSERT_NE(input, nullptr);
        ASSERT_GE(fputs(s, input), 0);
        rewind(input);
    }

    Token scanner_get_token() const
    {
        return get_token(scanner_context, input);
    }
};

// =====================================================

TEST_F(scanner, err_invalid_char)
{
    scanner_input("\0");
    const Token t = scanner_get_token();

    ASSERT_EQ(t.type, Token::ERROR);
}

// =====================================================
// Integer tests

TEST_F(scanner, int_simple)
{
    scanner_input("123");
    const Token t = scanner_get_token();

    ASSERT_EQ(t.type, Token::INTEGER);
    EXPECT_EQ(t.attribute_int, 123);
}

TEST_F(scanner, int_underscores)
{
    scanner_input("1_23_455");
    const Token t = scanner_get_token();

    ASSERT_EQ(t.type, Token::INTEGER);
    EXPECT_EQ(t.attribute_int, 123455);
}

TEST_F(scanner, int_zero)
{
    scanner_input("0");
    const Token t = scanner_get_token();

    ASSERT_EQ(t.type, Token::INTEGER);
    EXPECT_EQ(t.attribute_int, 0);
}

// Integer error handling tests

TEST_F(scanner, int_err_leading_zero)
{
    scanner_input("0123");
    const Token t = scanner_get_token();

    ASSERT_EQ(t.type, Token::ERROR);
}

// =====================================================