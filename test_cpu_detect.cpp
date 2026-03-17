#include <iostream>
#include <thread>
#include <algorithm>

int main()
{
    unsigned int cores = std::thread::hardware_concurrency();
    std::cout << "std::thread::hardware_concurrency() = " << cores << std::endl;

    // Old formula
    unsigned int oldThreads = cores > 2 ? cores - 2 : 1;
    std::cout << "Old formula result: " << oldThreads << std::endl;

    // New formula (my fix)
    unsigned int newThreads = std::max(4u, cores > 6 ? cores - 2 : 4);
    std::cout << "New formula result: " << newThreads << std::endl;

    return 0;
}
