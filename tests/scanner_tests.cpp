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

    /**
     * @brief Writes a given string to the scanner input stream
     * @param string the input string to be written at the start of the stream
     */
    void scanner_input(const char* string)
    {
        tmp_input = tmpfile();
        ASSERT_NE(tmp_input, nullptr);
        ASSERT_GE(fputs(string, tmp_input), 0);
        rewind(tmp_input);

        init_scanner(&scanner_context);
        init_shared_data(&data);
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
                case IDENTIFIER:
                    EXPECT_STREQ(get_identifier(&data, token.attribute_str_idx), desired_token.attribute_str_ptr);
                    break;
                case INTEGER:
                    EXPECT_EQ(token.attribute_int, desired_token.attribute_int);
                    break;
                case DOUBLE:
                    EXPECT_EQ(token.attribute_double, desired_token.attribute_double);
                    break;
                case BOOLEAN:
                    EXPECT_EQ(token.attribute_bool, desired_token.attribute_bool);
                    break;
                default:
                    break;
            }
        }

        if (tmp_input != nullptr) {
            fclose(tmp_input);
        }

        destroy_scanner(&scanner_context);
        destroy_shared_data(&data);
    }

    // Token constructors for readability
    static Token token(const type_enum type)
    {
        return { .type = type };
    }
    static Token token(const type_enum type, const char* value)
    {
        return { .type = type, .attribute_str_ptr = value };
    }
    static Token token(const type_enum type, const int value)
    {
        return { .type = type, .attribute_int = value };
    }
    static Token token(const type_enum type, const double value)
    {
        return { .type = type, .attribute_double = value };
    }
};

// =====================================================

TEST_F(scanner, end_of_file)
{
    verify_token_sequence("",
        {
            token(LINE_FEED),
            token(END_OF_FILE),
        });
}

TEST_F(scanner, err_invalid_char)
{
    verify_token_sequence("%",
        {
            token(ERROR),
        });
}

// =====================================================
// Identifier tests

TEST_F(scanner, identifier_lowercase)
{
    verify_token_sequence("abcdefghijklmnopqrstuvwxyz",
        {
            token(IDENTIFIER, "abcdefghijklmnopqrstuvwxyz"),
        });
}

TEST_F(scanner, identifier_uppercase)
{
    verify_token_sequence("ABCDEFGHIJKLMOPQRSTUVWXYZ",
        {
            token(IDENTIFIER, "ABCDEFGHIJKLMOPQRSTUVWXYZ"),
        });
}

TEST_F(scanner, identifier_underscores)
{
    verify_token_sequence("abcd_def__gh__i___",
        {
            token(IDENTIFIER, "abcd_def__gh__i___"),
        });

    verify_token_sequence("_abcd",
        {
            token(IDENTIFIER, "_abcd"),
        });
}

TEST_F(scanner, identifier_numbers)
{
    verify_token_sequence("a123456789",
        {
            token(IDENTIFIER, "a123456789"),
        });

    verify_token_sequence("_123456789",
        {
            token(IDENTIFIER, "_123456789"),
        });
}

TEST_F(scanner, identifier_combinations)
{
    verify_token_sequence("_identifier49812",
        {
            token(IDENTIFIER, "_identifier49812"),
        });

    verify_token_sequence("t1_",
        {
            token(IDENTIFIER, "t1_"),
        });

    verify_token_sequence("_1t_3t4_t_5_b",
        {
            token(IDENTIFIER, "_1t_3t4_t_5_b"),
        });
}

TEST_F(scanner, identifier_multiple)
{
    verify_token_sequence("ERROR _counter x1 y z",
        {
            token(IDENTIFIER, "ERROR"),
            token(IDENTIFIER, "_counter"),
            token(IDENTIFIER, "x1"),
            token(IDENTIFIER, "y"),
            token(IDENTIFIER, "z"),
        });
}

// Error handling tests

TEST_F(scanner, identifier_err_single_underscore)
{
    verify_token_sequence("_",
        {
            token(ERROR),
        });
}

// =====================================================
// Integer tests

TEST_F(scanner, int_simple)
{
    verify_token_sequence("123",
        {
            token(INTEGER, 123),
        });
}

TEST_F(scanner, int_underscores)
{
    verify_token_sequence("1_23_455",
        {
            token(INTEGER, 123455),
        });
}

TEST_F(scanner, int_zero)
{
    verify_token_sequence("0",
        {
            token(INTEGER, 0),
        });
}

// Error handling tests

TEST_F(scanner, int_err_leading_zero)
{
    verify_token_sequence("0123",
        {
            token(ERROR),
        });

    verify_token_sequence("0_1_2_3",
        {
            token(ERROR),
        });
}

// =====================================================
// Double tests

TEST_F(scanner, double_simple)
{
    verify_token_sequence("1.12",
        {
            token(DOUBLE, 1.12),
        });
}

TEST_F(scanner, double_zero_after_dot)
{
    verify_token_sequence("1.012",
        {
            token(DOUBLE, 1.012),
        });
}

TEST_F(scanner, double_zero_before_dot)
{
    verify_token_sequence("0.12",
        {
            token(DOUBLE, 0.12),
        });
}

TEST_F(scanner, double_but_int)
{
    verify_token_sequence("1.0",
        {
            token(DOUBLE, 1.0),
        });
}

TEST_F(scanner, double_two_dots)
{
    verify_token_sequence("1.1.12",
        {
            token(DOUBLE, 1.1),
            token(DOUBLE, 0.12)
        });
}

TEST_F(scanner, double_no_full_number)
{
    verify_token_sequence(".23",
        {
            token(DOUBLE, .23)
        });
}

TEST_F(scanner, double_no_decimal)
{
    verify_token_sequence("1.",
        {
            token(ERROR)
        });
}

TEST_F(scanner, double_with_d)
{
    verify_token_sequence("1.2d",
        {
            token(DOUBLE, 1.2)
        });

    verify_token_sequence("1.2D",
        {
            token(DOUBLE, 1.2)
        });
}

TEST_F(scanner, double_without_full_with_d)
{
    verify_token_sequence(".432d",
        {
            token(DOUBLE, .432)
        });

    verify_token_sequence(".432D",
        {
            token(DOUBLE, .432)
        });
}

TEST_F(scanner, double_with_underscores)
{
    verify_token_sequence("1_134_123.24d",
        {
            token(DOUBLE, 1134123.24)
        });
}

TEST_F(scanner, double_with_underscores_in_decimal)
{
    verify_token_sequence("1_134_123.242_029",
        {
            token(DOUBLE, 1134123.242029)
        });

    verify_token_sequence("11_34_12_3.2_4_202_9D",
        {
            token(DOUBLE, 1134123.242029)
        });
}

TEST_F(scanner, double_with_underscores_without_full)
{
    verify_token_sequence(".242_029",
        {
            token(DOUBLE, .242029)
        });
}

// =====================================================
// Double tests with exponents

TEST_F(scanner, double_with_negative_exponent)
{
    verify_token_sequence("12.3E-2",
        {
            token(DOUBLE, 0.123)
        });
}

TEST_F(scanner, double_with_positive_exponent)
{
    verify_token_sequence("12.323E+2",
        {
            token(DOUBLE, 1232.3)
        });

    verify_token_sequence("12.323E2",
        {
            token(DOUBLE, 1232.3)
        });
}

TEST_F(scanner, double_exp_no_full_number)
{
    verify_token_sequence(".31E-2",
        {
            token(DOUBLE, 0.0031)
        });

    verify_token_sequence(".312E2",
        {
            token(DOUBLE, 31.2)
        });
}

TEST_F(scanner, double_exp_err_no_decimal_number)
{
    verify_token_sequence(".E-2",
        {
            token(DOT, ".E-2")
        });

    verify_token_sequence(".E2",
        {
            token(DOT, ".E2")
        });
}

TEST_F(scanner, double_exp_full_number)
{
    verify_token_sequence("1E-2",
        {
            token(DOUBLE, 0.01)
        });

    verify_token_sequence("1E+2",
        {
            token(DOUBLE, 100.0)
        });
}