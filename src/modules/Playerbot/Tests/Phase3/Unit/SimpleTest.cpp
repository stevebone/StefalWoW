/*
 * Simple diagnostic test to verify GTest integration
 */

#include <gtest/gtest.h>

// Simplest possible test - if this doesn't register, GTest integration is broken
TEST(SimpleTest, AlwaysPass)
{
    EXPECT_EQ(1, 1);
}

TEST(SimpleTest, BasicAssertion)
{
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}
