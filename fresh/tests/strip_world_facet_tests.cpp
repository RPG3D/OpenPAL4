#include "pal4/strip_world_facet.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IStripWorldFacetRuntime {
    pal4::StripObjectState* state = nullptr;
    std::vector<std::string> events;
    std::uint32_t world = 0x5000u;
    std::uint32_t ResolveWorld(std::uint32_t clump) override {
        Check(clump == 0x4000u &&
                  ((state->words[1u] >> 16u) & 0xffffu) == 0u,
              "world lookup follows started and display clear");
        events.emplace_back("world");
        return world;
    }
    std::uint8_t RemoveClump(std::uint32_t value,
                             std::uint32_t clump) override {
        Check(value == 0x5000u && clump == 0x4000u,
              "remove resolved world clump");
        events.emplace_back("remove");
        return 0x5au;
    }
};

void TestRemove() {
    pal4::StripObjectState state{};
    state.words[1u] = 0x01010001u;
    state.words[34u] = 0x4000u;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::StripWorldFacet::Remove(state, runtime) == 0x5au &&
              state.words[1u] == 1u &&
              runtime.events == std::vector<std::string>{"world", "remove"},
          "active facet removes clump after flag cleanup");

    state.words[1u] = 0x01010001u;
    runtime.events.clear();
    runtime.world = 0u;
    Check(pal4::StripWorldFacet::Remove(state, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"world"} &&
              state.words[1u] == 1u,
          "missing world still clears started and display state");

    state.words[1u] = 0x01010000u;
    runtime.events.clear();
    Check(pal4::StripWorldFacet::Remove(state, runtime) == 0u &&
              runtime.events.empty() && state.words[1u] == 0x01010000u,
          "inactive owner preserves state");
    state.words[1u] = 0x01000001u;
    Check(pal4::StripWorldFacet::Remove(state, runtime) == 0u &&
              runtime.events.empty() && state.words[1u] == 0x01000001u,
          "unstarted facet preserves display byte");
}

}  // namespace

int main() {
    TestRemove();
}
