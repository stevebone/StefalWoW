/*
 * Custom main() for playerbot tests
 *
 * MSVC's linker is very aggressive about removing "unreferenced" code,
 * including GTest's static test registration. This custom main() ensures
 * all test object files are actually linked in.
 */

#include <gtest/gtest.h>
#include <iostream>

int main(int argc, char** argv)
{
    std::cout << "PlayerBot Test Suite - Custom main()" << std::endl;
    std::cout << "Initializing Google Test..." << std::endl;

    ::testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();

    std::cout << "Tests completed with result: " << result << std::endl;

    return result;
}
