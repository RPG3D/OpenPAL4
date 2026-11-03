#include "pal4/game_data_table_slot.h"

#include <array>
#include <cstdint>
#include <iostream>

int main() {
    pal4::GameDataTableSlot slot;
    slot.words = {1, 2, 3, 4, 5};
    if (&slot.Initialize() != &slot ||
        slot.words != std::array<std::uint32_t, 5>{}) {
        std::cerr << "table slot must clear all five DWORDs and return self\n";
        return 1;
    }
    slot.words[4] = 0xffffffffu;
    if (&slot.Initialize() != &slot || slot.words[4] != 0) {
        std::cerr << "repeated table slot initialization failed\n";
        return 1;
    }
    std::cout << "game_data_table_slot_tests passed\n";
}
