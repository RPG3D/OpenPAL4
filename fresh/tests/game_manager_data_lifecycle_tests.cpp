#include "pal4/game_manager_data_lifecycle.h"

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

struct Memory final : pal4::IGameManagerDataMemory {
    pal4::GameManagerDataState& state;
    std::vector<std::uint32_t> freed;

    explicit Memory(pal4::GameManagerDataState& image) : state(image) {}
    void Free(std::uint32_t address) override {
        if (freed.empty()) {
            Check(address == 0x2000u &&
                      state.words[0] ==
                          pal4::GameManagerDataLifecycle::kCleanupVtable &&
                      state.words[0x2b8u / 4u] == 0x2000u &&
                      state.words[0x2a8u / 4u] == 0x1000u,
                  "first free precedes both buffer clears");
        } else if (freed.size() == 1u) {
            Check(address == 0x1000u &&
                      state.words[0x2b8u / 4u] == 0u &&
                      state.words[0x2bcu / 4u] == 0u &&
                      state.words[0x2c0u / 4u] == 0u &&
                      state.words[0x2a8u / 4u] == 0x1000u,
                  "second free follows first triple clear");
        } else {
            Check(address == 0x5000u &&
                      state.words[0] ==
                          pal4::GameManagerDataLifecycle::kBaseVtable &&
                      state.words[0x2a8u / 4u] == 0u,
                  "object free follows base-vtable restoration");
        }
        freed.push_back(address);
    }
};

pal4::GameManagerDataState MakeState() {
    pal4::GameManagerDataState state{};
    state.words.fill(0xa5a5a5a5u);
    state.words[0x2a8u / 4u] = 0x1000u;
    state.words[0x2b8u / 4u] = 0x2000u;
    return state;
}

void TestCleanup() {
    auto state = MakeState();
    Memory memory(state);
    pal4::GameManagerDataLifecycle::Cleanup(state, memory);
    Check(memory.freed == std::vector<std::uint32_t>({0x2000u, 0x1000u}) &&
              state.words[0] == pal4::GameManagerDataLifecycle::kBaseVtable &&
              state.words[0x2a8u / 4u] == 0u &&
              state.words[0x2acu / 4u] == 0u &&
              state.words[0x2b0u / 4u] == 0u &&
              state.words[0x2b8u / 4u] == 0u &&
              state.words[0x2bcu / 4u] == 0u &&
              state.words[0x2c0u / 4u] == 0u &&
              state.words[0x2b4u / 4u] == 0xa5a5a5a5u,
          "cleanup clears only observed triples and restores base vtable");
}

void TestDeletingDestructor() {
    for (const auto flags : {0u, 1u, 2u, 3u}) {
        auto state = MakeState();
        Memory memory(state);
        Check(pal4::GameManagerDataLifecycle::DestroyDeleting(
                  state, 0x5000u, flags, memory) == 0x5000u,
              "deleting destructor returns original object");
        const std::vector<std::uint32_t> expected =
            (flags & 1u) != 0u
                ? std::vector<std::uint32_t>{0x2000u, 0x1000u, 0x5000u}
                : std::vector<std::uint32_t>{0x2000u, 0x1000u};
        Check(memory.freed == expected, "only low flag bit frees object");
    }
}

}  // namespace

int main() {
    TestCleanup();
    TestDeletingDestructor();
}
