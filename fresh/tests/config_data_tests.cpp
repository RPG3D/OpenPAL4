#include "pal4/config_data.h"

#include <cstdlib>
#include <iostream>

int main() {
    pal4::ConfigDataState state;
    state.words.fill(0x12345678u);
    if (&pal4::ConfigData::Initialize(state) != &state) {
        std::cerr << "initialization did not return original object\n";
        return EXIT_FAILURE;
    }
    for (const auto word : state.words) {
        if (word != 0u) {
            std::cerr << "initialization left a nonzero word\n";
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
