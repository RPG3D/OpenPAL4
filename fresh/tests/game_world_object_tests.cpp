#include "pal4/game_world_object.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class DictionaryRuntime final : public pal4::IGameWorldDictionaryRuntime {
public:
    void InitializeDictionary(pal4::GameWorldObjectState& state,
                              const std::uint32_t offset) override {
        calls.push_back(offset);
        state.words[offset] = 0;
        state.words[offset + 1] = 0;
        state.words[offset + 2] = 0x3f800000u;
    }
    void CleanupDictionary(pal4::GameWorldObjectState& state,
                           const std::uint32_t offset) override {
        Check(state.words[0] == pal4::GameWorldObject::kVtableAddress,
              "destructor installs vtable before nested cleanup");
        calls.push_back(offset);
    }
    std::vector<std::uint32_t> calls;
};

void Lifecycle() {
    static_assert(sizeof(pal4::GameWorldObjectState) == 196);
    pal4::GameWorldObjectState state;
    state.words.fill(0xdeadbeefu);
    DictionaryRuntime runtime;
    Check(&pal4::GameWorldObject::Construct(state, runtime) == &state,
          "constructor returns original object");
    Check(runtime.calls == std::vector<std::uint32_t>({7, 14, 19, 24, 29, 34}),
          "six dictionaries initialize in forward order");
    for (const auto offset : runtime.calls)
        Check(state.words[offset] == 0 && state.words[offset + 1] == 0 &&
                  state.words[offset + 2] == 0x3f800000u,
              "each dictionary contains native initial fields");
    constexpr std::array<std::uint32_t, 19> cleared{
        1, 2, 3, 4, 5, 6, 11, 12, 13, 18, 23, 28, 33,
        45, 46, 47, 48, 42, 43};
    for (const auto offset : cleared)
        Check(state.words[offset] == 0, "constructor clears observed fields");
    constexpr std::array<std::uint32_t, 6> enabled{10, 17, 22, 27, 32, 41};
    for (const auto offset : enabled)
        Check(state.words[offset] == 1, "constructor enables observed fields");
    Check(state.words[0] == pal4::GameWorldObject::kVtableAddress &&
              state.words[44] == pal4::GameWorldObject::kDefaultParameterBits,
          "constructor writes vtable and default parameter bits");
    Check(state.words[37] == 0xdeadbeefu,
          "constructor preserves untouched object words");

    runtime.calls.clear();
    state.words[0] = 0;
    pal4::GameWorldObject::Destruct(state, runtime);
    Check(runtime.calls ==
              std::vector<std::uint32_t>({34, 29, 24, 19, 14, 7}),
          "destructor cleans dictionaries in reverse order");
}

}  // namespace

int main() {
    Lifecycle();
}
