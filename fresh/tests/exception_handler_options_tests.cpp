#include "pal4/exception_handler_options.h"

#include <array>
#include <cstdlib>
#include <iostream>

int main() {
    pal4::ExceptionHandlerOptions options;
    if (options.Set(1u, 2u, 3u, 4u, 5u) != 4u ||
        options.Values() != std::array<std::uint32_t, 5>{1u, 2u, 3u, 4u, 5u} ||
        options.Set(0u, 0u, 0u, 0u, 0u) != 0u ||
        options.Values() != std::array<std::uint32_t, 5>{}) {
        std::cerr << "FAIL: five exception options and fourth-argument return\n";
        return EXIT_FAILURE;
    }
    std::cout << "exception handler options test passed\n";
}
