#include "pal4/crt_placement_new.h"

#include <cstdlib>
#include <iostream>

int main() {
    if (pal4::CrtPlacementNew::At(0u, 0x1234u) != 0x1234u ||
        pal4::CrtPlacementNew::At(4096u, 0x1234u) != 0x1234u ||
        pal4::CrtPlacementNew::At(0xffffffffu, 0u) != 0u) {
        std::cerr << "placement new must return supplied address\n";
        return EXIT_FAILURE;
    }
    std::cout << "crt_placement_new_tests passed\n";
}
