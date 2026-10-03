/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    test.cpp
 * @brief
 * @author
 */

#include <gtest/gtest.h>

extern "C" {
#include "scanner.h"
}

TEST(Scanner, ReturnNothing)
{
    EXPECT_EQ(0, get_token());
}