#include "pal4/game_config_state.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory final : public pal4::IGameConfigMemory {
public:
    std::uint32_t Free(std::uint32_t address) override {
        freed.push_back(address);
        return address + 1;
    }
    std::vector<std::uint32_t> freed;
};

void FreeAndNull() {
    Memory memory;
    std::uint32_t slot = 0;
    Check(pal4::GameConfigState::FreeAndNull(slot, memory) == 0 &&
          memory.freed.empty(), "null pointer is not freed");
    slot = 0x120;
    Check(pal4::GameConfigState::FreeAndNull(slot, memory) == 0x121 &&
          slot == 0 && memory.freed == std::vector<std::uint32_t>{0x120},
          "non-null pointer is freed and nulled");
}

void InitializeAndCleanup() {
    pal4::GameConfigState state;
    state.words.fill(0xffffffffu);
    Check(&state.Initialize() == &state, "initialization returns object");
    for (std::size_t i = 0; i < state.words.size(); ++i) {
        const auto expected = i == 6 || i == 7 ? 0x3f800000u :
                              i == 13 ? 3u : 0u;
        Check(state.words[i] == expected, "all default words match IDA");
    }
    state.words.fill(0xdeadbeefu);
    state.words[9] = 0x10;
    state.words[10] = 0;
    state.words[11] = 0x30;
    state.words[12] = 0x40;
    state.words[14] = 0x55;
    Memory memory;
    Check(state.Cleanup(memory) == 0x55 &&
          memory.freed == std::vector<std::uint32_t>{0x10, 0x30, 0x40},
          "cleanup frees four optional slots in order and returns old word 14");
    for (std::size_t i = 0; i < state.words.size(); ++i) {
        const auto expected = i <= 5 || (i >= 9 && i <= 12) || i >= 14 ? 0u :
                              i == 13 ? 3u : 0xdeadbeefu;
        Check(state.words[i] == expected, "cleanup touches only IDA fields");
    }
}

}  // namespace

int main() {
    FreeAndNull();
    InitializeAndCleanup();
}
