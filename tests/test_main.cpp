/**
 * @project Implementation of the IFJ26 Imperative Language Compiler
 * @file    test_main.cpp
 * @brief   Automated unit test starting point
 * @author  Filip Hajdusek (xhajduf00)
 */

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}