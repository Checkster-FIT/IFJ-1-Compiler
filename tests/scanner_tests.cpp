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
    FILE* tmp_input = nullptr; // Temporary input file for stdin emulation
    ScannerContext scanner_context = { };
    SharedData data = { };

    /** @brief Prepares an input stream for each scanner test */
    void SetUp() override
    {
        tmp_input = tmpfile();
        ASSERT_NE(tmp_input, nullptr);

        init_scanner(&scanner_context);
        init_shared_data(&data);
    }

    /** @brief Closes the input stream after each scanner test */
    void TearDown() override
    {
        if (tmp_input != nullptr) {
            fclose(tmp_input);
        }

        destroy_scanner(&scanner_context);
        destroy_shared_data(&data);
    }

    /**
     * @brief Writes a given string to the scanner input stream
     * @param string the input string to be written at the start of the stream
     */
    void scanner_input(const char* string) const
    {
        ASSERT_NE(tmp_input, nullptr);
        ASSERT_GE(fputs(string, tmp_input), 0);
        rewind(tmp_input);
    }

    /**
     * @brief Compares the scanner output with a list of expected tokens
     * (checks the token type and the token attribute value if possible)
     * @param string the string to be read by the scanner
     * @param expected_tokens a list of expected tokens from the scanner
     */
    void verify_token_sequence(
        const char* string,
        const std::initializer_list<Token> expected_tokens)
    {
        scanner_input(string);
        for (const auto desired_token : expected_tokens) {

            const Token token = get_token(&scanner_context, &data, tmp_input);
            // printf("Token type: %d\n", token.type);

            EXPECT_EQ(token.type, desired_token.type);

            switch (token.type) {
                case Token::IDENTIFIER:
                    EXPECT_STREQ(get_identifier(&data, token.attribute_str_idx), desired_token.attribute_str_ptr);
                    break;
                case Token::INTEGER:
                    EXPECT_EQ(token.attribute_int, desired_token.attribute_int);
                    break;
                case Token::DOUBLE:
                    EXPECT_EQ(token.attribute_double, desired_token.attribute_double);
                    break;
                case Token::BOOLEAN:
                    EXPECT_EQ(token.attribute_bool, desired_token.attribute_bool);
                default:
                    break;
            }
        }
    }

    // Token constructors for readability
    static Token token(const Token::type_enum type)
    {
        return { .type = type };
    }
    static Token token(const Token::type_enum type, const char* value)
    {
        return { .type = type, .attribute_str_ptr = value };
    }
    static Token token(const Token::type_enum type, const int value)
    {
        return { .type = type, .attribute_int = value };
    }
    static Token token(const Token::type_enum type, const double value)
    {
        return { .type = type, .attribute_double = value };
    }
};

// =====================================================

TEST_F(scanner, end_of_file)
{
    verify_token_sequence("",
        {
            token(Token::END_OF_FILE),
        });
}

TEST_F(scanner, err_invalid_char)
{
    verify_token_sequence("%",
        {
            token(Token::ERROR),
        });
}

// =====================================================
// Identifier tests

TEST_F(scanner, identifier_lowercase)
{
    verify_token_sequence("abcdefghijklmnopqrstuvwxyz",
        {
            token(Token::IDENTIFIER, "abcdefghijklmnopqrstuvwxyz"),
        });
}

TEST_F(scanner, identifier_uppercase)
{
    verify_token_sequence("ABCDEFGHIJKLMOPQRSTUVWXYZ",
        {
            token(Token::IDENTIFIER, "ABCDEFGHIJKLMOPQRSTUVWXYZ"),
        });
}

TEST_F(scanner, identifier_underscores)
{
    verify_token_sequence("abcd_def__gh__i___",
        {
            token(Token::IDENTIFIER, "abcd_def__gh__i___"),
        });

    verify_token_sequence("_abcd",
        {
            token(Token::IDENTIFIER, "_abcd"),
        });
}

TEST_F(scanner, identifier_numbers)
{
    verify_token_sequence("a123456789",
        {
            token(Token::IDENTIFIER, "a123456789"),
        });

    verify_token_sequence("_123456789",
        {
            token(Token::IDENTIFIER, "_123456789"),
        });
}

TEST_F(scanner, identifier_combinations)
{
    verify_token_sequence("_identifier49812",
        {
            token(Token::IDENTIFIER, "_identifier49812"),
        });

    verify_token_sequence("t1_",
        {
            token(Token::IDENTIFIER, "t1_"),
        });

    verify_token_sequence("_1t_3t4_t_5_b",
        {
            token(Token::IDENTIFIER, "_1t_3t4_t_5_b"),
        });
}

TEST_F(scanner, identifier_multiple)
{
    verify_token_sequence("ERROR _counter x1 y z",
        {
            token(Token::IDENTIFIER, "ERROR"),
            token(Token::IDENTIFIER, "_counter"),
            token(Token::IDENTIFIER, "x1"),
            token(Token::IDENTIFIER, "y"),
            token(Token::IDENTIFIER, "z"),
        });
}

// Error handling tests

TEST_F(scanner, identifier_err_single_underscore)
{
    verify_token_sequence("_",
        {
            token(Token::ERROR),
        });
}

// =====================================================
// Integer tests

TEST_F(scanner, int_simple)
{
    verify_token_sequence("123",
        {
            token(Token::INTEGER, 123),
        });
}

TEST_F(scanner, int_underscores)
{
    verify_token_sequence("1_23_455",
        {
            token(Token::INTEGER, 123455),
        });
}

TEST_F(scanner, int_zero)
{
    verify_token_sequence("0",
        {
            token(Token::INTEGER, 0),
        });
}

// Error handling tests

TEST_F(scanner, int_err_leading_zero)
{
    verify_token_sequence("0123",
        {
            token(Token::ERROR),
        });

    verify_token_sequence("0_1_2_3",
        {
            token(Token::ERROR),
        });
}

// =====================================================