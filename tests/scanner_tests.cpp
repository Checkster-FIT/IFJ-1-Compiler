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
    AllocatedStrings* strings = { };
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

        strings = new_strings();
        if (strings == nullptr)
            return;

        init_scanner(&scanner_context, strings);
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
            const Token token = get_token(&scanner_context, tmp_input);
            // printf("Token type: %d\n", token.type);

            EXPECT_EQ(token.type, desired_token.type);

            switch (token.type) {
                case IDENTIFIER:
                    EXPECT_STREQ(token.attribute_string, desired_token.attribute_string);
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
                case STRING:
                    EXPECT_STREQ(token.attribute_string, desired_token.attribute_string);
                    break;
                default:
                    break;
            }
        }

        if (tmp_input != nullptr) {
            fclose(tmp_input);
        }

        destroy_scanner(&scanner_context);
    }

    // Token constructors for readability
    static Token token(const type_enum type)
    {
        return { .type = type };
    }

    static Token token(const type_enum type, const char* value)
    {
        return { .type = type, .attribute_string = value };
    }

    static Token token(const type_enum type, const int value)
    {
        return { .type = type, .attribute_int = value };
    }

    static Token token(const type_enum type, const double value)
    {
        return { .type = type, .attribute_double = value };
    }

    void SetUp() override
    {
        testing::internal::CaptureStderr();
    }

    void TearDown() override
    {
        testing::internal::GetCapturedStderr();
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
    verify_token_sequence("00",
        {
            token(INTEGER, 0),
        });
}

TEST_F(scanner, underscore)
{
    verify_token_sequence("_", { token(ERROR) });
}

TEST_F(scanner, int_two_literals)
{
    verify_token_sequence("1 2", { token(INTEGER, 1), token(INTEGER, 2) });
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

TEST_F(scanner, zerozeroone)
{
    verify_token_sequence("001", { token(ERROR) });
}

TEST_F(scanner, int_err_trailing_underscore)
{
    verify_token_sequence("1_", { token(ERROR) });
}

// ===================== Doubles =====================

TEST_F(scanner, double_simple)
{
    verify_token_sequence("1.5", { token(DOUBLE, 1.5) });
    verify_token_sequence("0.5", { token(DOUBLE, 0.5) });
    verify_token_sequence("0.0", { token(DOUBLE, 0.0) });
}

TEST_F(scanner, double_missing_integer_part)
{
    verify_token_sequence(".5", { token(DOUBLE, 0.5) });
    verify_token_sequence(".5e2", { token(DOUBLE, 50.0) });
}

TEST_F(scanner, double_exponent)
{
    verify_token_sequence("1e3", { token(DOUBLE, 1000.0) });
    verify_token_sequence("1E3", { token(DOUBLE, 1000.0) });
    verify_token_sequence("1e+3", { token(DOUBLE, 1000.0) });
    verify_token_sequence("1e-3", { token(DOUBLE, 0.001) });
    verify_token_sequence("1.5e3", { token(DOUBLE, 1500.0) });
}

TEST_F(scanner, double_underscores)
{
    verify_token_sequence("1_0.2_5", { token(DOUBLE, 10.25) });
    verify_token_sequence("1e1_0", { token(DOUBLE, 1e10) });
}

TEST_F(scanner, double_suffix)
{
    verify_token_sequence("1.5d", { token(DOUBLE, 1.5) });
    verify_token_sequence("1.5D", { token(DOUBLE, 1.5) });
    verify_token_sequence("1e3d", { token(DOUBLE, 1000.0) });
    verify_token_sequence(".5d", { token(DOUBLE, 0.5) });
}

TEST_F(scanner, double_err_incomplete_exponent)
{
    verify_token_sequence("1e", { token(ERROR) });
    verify_token_sequence("1e+", { token(ERROR) });
    verify_token_sequence("1.5e-", { token(ERROR) });
}

TEST_F(scanner, double_err_underscore_placement)
{
    verify_token_sequence("1_.5", { token(ERROR) });
    verify_token_sequence("1.5_", { token(ERROR) });
}

// ===================== Strings =====================
TEST_F(scanner, string_simple)
{
    verify_token_sequence("\"adam zapletal\"", { token(STRING, "adam zapletal") });
    verify_token_sequence("\"\"", { token(STRING, "") });
}

TEST_F(scanner, string_special_chars_inside)
{
    verify_token_sequence("\"a b # $ ' //x /*y*/\"", { token(STRING, "a b # $ ' //x /*y*/") });
}

TEST_F(scanner, string_escape_backslash)
{
    verify_token_sequence("\"\\\\\"", { token(STRING, "\\") });
}

TEST_F(scanner, string_escape_quote)
{
    verify_token_sequence("\"\\\"\"", { token(STRING, "\"") });
}

TEST_F(scanner, string_escape_newline)
{
    verify_token_sequence("\"\\n\"", { token(STRING, "\n") });
}

TEST_F(scanner, string_escape_carriage_return)
{
    verify_token_sequence("\"\\r\"", { token(STRING, "\r") });
}

TEST_F(scanner, string_escape_tab)
{
    verify_token_sequence("\"\\t\"", { token(STRING, "\t") });
}

TEST_F(scanner, string_escape_complex_sequence)
{
    verify_token_sequence("\"a\\n\\t\\\\\\\"b\"", { token(STRING, "a\n\t\\\"b") });
}

TEST_F(scanner, string_adjacent)
{
    verify_token_sequence("\"a\"\"b\"", { token(STRING, "a"), token(STRING, "b") });
    verify_token_sequence("\"a\" \"b\"", { token(STRING, "a"), token(STRING, "b") });
}

TEST_F(scanner, string_err_real_newline)
{
    verify_token_sequence("\"abc\ndef\"", { token(ERROR) });
}

// ===================== Multiline strings =====================

TEST_F(scanner, mstring_simple)
{
    verify_token_sequence("\"\"\"abc\"\"\"", { token(MULTILINE_STRING, "abc") });
    verify_token_sequence("\"\"\"\"\"\"", { token(MULTILINE_STRING, "") });
}

TEST_F(scanner, mstring_quotes_inside)
{
    verify_token_sequence("\"\"\"a\"b\"\"\"", { token(MULTILINE_STRING, "a\"b") });
    verify_token_sequence("\"\"\"a\"\"b\"\"\"", { token(MULTILINE_STRING, "a\"\"b") });
}

// ===================== Identifiers (keyword prefixes)
TEST_F(scanner, identifier_keyword_lookalikes)
{
    verify_token_sequence("iff defx val_ _val Intt whilee int string main Double1",
        {
            token(IDENTIFIER, "iff"),
            token(IDENTIFIER, "defx"),
            token(IDENTIFIER, "val_"),
            token(IDENTIFIER, "_val"),
            token(IDENTIFIER, "Intt"),
            token(IDENTIFIER, "whilee"),
            token(IDENTIFIER, "int"),
            token(IDENTIFIER, "string"),
            token(IDENTIFIER, "main"),
            token(IDENTIFIER, "Double1"),
        });
}

// =====================================================
// Double tests

TEST_F(scanner, double_simple2)
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
        { token(DOUBLE, 1.1),
            token(DOUBLE, 0.12) });
}

TEST_F(scanner, double_no_full_number)
{
    verify_token_sequence(".23",
        { token(DOUBLE, .23) });
}

TEST_F(scanner, double_no_decimal)
{
    verify_token_sequence("1.",
        { token(ERROR) });
}

TEST_F(scanner, double_with_d)
{
    verify_token_sequence("1.2d",
        { token(DOUBLE, 1.2) });

    verify_token_sequence("1.2D",
        { token(DOUBLE, 1.2) });
}

TEST_F(scanner, double_without_full_with_d)
{
    verify_token_sequence(".432d",
        { token(DOUBLE, .432) });

    verify_token_sequence(".432D",
        { token(DOUBLE, .432) });
}

TEST_F(scanner, double_with_underscores)
{
    verify_token_sequence("1_134_123.24d",
        { token(DOUBLE, 1134123.24) });
}

TEST_F(scanner, double_with_underscores_in_decimal)
{
    verify_token_sequence("1_134_123.242_029",
        { token(DOUBLE, 1134123.242029) });

    verify_token_sequence("11_34_12_3.2_4_202_9D",
        { token(DOUBLE, 1134123.242029) });
}

TEST_F(scanner, double_with_underscores_without_full)
{
    verify_token_sequence(".242_029",
        { token(DOUBLE, .242029) });
}

// =====================================================
// Double tests with exponents

TEST_F(scanner, double_with_negative_exponent)
{
    verify_token_sequence("12.3E-2",
        { token(DOUBLE, 0.123) });
}

TEST_F(scanner, double_with_positive_exponent)
{
    verify_token_sequence("12.323E+2",
        { token(DOUBLE, 1232.3) });

    verify_token_sequence("12.323E2",
        { token(DOUBLE, 1232.3) });
}

TEST_F(scanner, double_exp_no_full_number)
{
    verify_token_sequence(".31E-2",
        { token(DOUBLE, 0.0031) });

    verify_token_sequence(".312E2",
        { token(DOUBLE, 31.2) });
}

TEST_F(scanner, double_exp_err_no_decimal_number)
{
    verify_token_sequence(".E-2",
        { token(DOT, ".E-2") });

    verify_token_sequence(".E2",
        { token(DOT, ".E2") });
}

TEST_F(scanner, double_exp_full_number)
{
    verify_token_sequence("1E-2",
        { token(DOUBLE, 0.01) });

    verify_token_sequence("1E+2",
        { token(DOUBLE, 100.0) });
}