#include "pal4/grid_manager.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IGridManagerRuntime {
public:
    std::array<std::uint32_t, 3u> TransformBasis(
        pal4::GridManagerState& state,
        const std::array<std::uint32_t, 3u>& source) override {
        Check(source == std::array<std::uint32_t, 3u>{
                            0u, 0u, 0x3f800000u},
              "base constructor supplies +Z basis source");
        Check(state.base_words[0] ==
                  (basis_calls == 0u ? 0x84b780u : 0x84b6e8u),
              "base constructor uses both native vtables");
        ++basis_calls;
        return {0x3f800000u, 0u, 0u};
    }
    void SetParameter(pal4::GridManagerState&,
                      const std::uint32_t slot,
                      const std::uint32_t bits) override {
        parameters.push_back({slot, bits});
    }
    std::array<std::uint32_t, 3u> ZeroVector() override {
        return {0u, 0u, 0u};
    }
    std::uint32_t NextIdentifier() override { return identifier++; }
    void DestroyNested(const std::uint32_t address,
                       const bool free_self) override {
        Check(address == 0x2000u && free_self,
              "nested object receives virtual deleting destructor flag");
        events.push_back("destroy_nested");
    }
    void FreeSelf(pal4::GridManagerState&) override {
        events.push_back("free_self");
    }
    std::uint32_t Free(const std::uint32_t address) override {
        Check(address == 0x3000u, "owned memory address");
        events.push_back("free_memory");
        return 0x55u;
    }
    std::vector<std::string> events;
    std::vector<std::array<std::uint32_t, 2u>> parameters;
    std::uint32_t basis_calls = 0u;
    std::uint32_t identifier = 17u;
};

void ConstructionAndCleanup() {
    Runtime runtime;
    pal4::GridManagerState state{};
    state.nested_grid = 0xffffffffu;
    state.owned_memory = 0xffffffffu;
    state.update_enabled = 1u;
    Check(&pal4::GridManager::Construct(state, runtime) == &state &&
              runtime.basis_calls == 2u &&
              runtime.parameters ==
                  std::vector<std::array<std::uint32_t, 2u>>{
                      {84u, 0x3f800000u}, {104u, 0u},
                      {92u, 0x3f000000u}, {116u, 0x3dcccccdU},
                      {124u, 0x3f800000u}} &&
              runtime.identifier == 18u &&
              state.base_words[0] == 0x84b600u &&
              state.base_words[1] == 0x3f800000u &&
              state.base_words[5] == 0x3f800000u &&
              state.base_words[9] == 0x3f800000u &&
              state.base_words[22] == 17u &&
              state.nested_grid == 0u && state.owned_memory == 0u &&
              state.reserved_a == 0u && state.reserved_b == 0u &&
              state.update_enabled == 0u && state.reset_pending == 0u,
          "constructor rebuilds base basis, parameters and ID before derived fields");
    state.nested_grid = 0x2000u;
    state.owned_memory = 0x3000u;
    Check(&pal4::GridManager::Destroy(state, true, runtime) == &state &&
              runtime.events == std::vector<std::string>{
                  "destroy_nested", "free_memory", "free_self"} &&
              state.base_words[0] == 0x84b6e8u &&
              state.nested_grid == 0u && state.owned_memory == 0u,
          "deleting destructor honors nested, owned and self order");
    runtime.events.clear();
    Check(pal4::GridManager::FreeMemory(state, runtime) == 0u &&
              runtime.events.empty(),
          "empty owned pointer skips free");
    pal4::GridManager::Cleanup(state, runtime);
    Check(runtime.events.empty() && state.base_words[0] == 0x84b6e8u,
          "repeat cleanup skips released resources");
}

void NonDeletingDestructor() {
    Runtime runtime;
    pal4::GridManagerState state{};
    (void)pal4::GridManager::Construct(state, runtime);
    runtime.events.clear();
    (void)pal4::GridManager::Destroy(state, false, runtime);
    Check(runtime.events.empty() && state.base_words[0] == 0x84b6e8u,
          "nondeleting destructor leaves owner allocation untouched");
    state.owned_memory = 0x3000u;
    Check(pal4::GridManager::FreeMemory(state, runtime) == 0x55u &&
              state.owned_memory == 0u,
          "free-memory entry forwards runtime result");
}

}  // namespace

int main() {
    ConstructionAndCleanup();
    NonDeletingDestructor();
    std::cout << "Grid manager tests passed\n";
}
