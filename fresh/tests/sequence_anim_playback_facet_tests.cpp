#include "pal4/sequence_anim_playback_facet.h"

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

struct Runtime final : pal4::ISequenceAnimPlaybackFacetRuntime {
    std::vector<std::string> events;
    pal4::SequenceAnimObjectState* state = nullptr;
    std::uint32_t world = 0x6000u;
    std::uint32_t reads = 0u;
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(state != nullptr &&
                  (state->words[1u] & 0xffff0000u) == 0u,
              "running and pending cleared before native calls");
        ++reads;
        if (address == 0x3030u) {
            events.emplace_back("geometry");
            return 0x4000u;
        }
        Check(address == 0x4008u, "clump pointer at geometry +8");
        events.emplace_back("clump");
        return reads == 2u ? 0x5000u : 0x5001u;
    }
    std::uint32_t WorldForClump(std::uint32_t clump) override {
        Check(clump == 0x5000u, "first clump queried for world");
        events.emplace_back("world");
        return world;
    }
    std::uint8_t RemoveClump(std::uint32_t input_world,
                              std::uint32_t clump) override {
        Check(input_world == 0x6000u && clump == 0x5001u,
              "second clump read used for world removal");
        events.emplace_back("remove");
        return 0x7bu;
    }
};

}  // namespace

int main() {
    pal4::SequenceAnimObjectState state{};
    state.words[30u] = 0x3000u;
    Runtime runtime;
    runtime.state = &state;
    state.words[1u] = 0x01010000u;
    Check(pal4::SequenceAnimPlaybackFacet::Stop(state, runtime) == 0u &&
              runtime.events.empty(), "inactive owner gate");
    state.words[1u] = 0x01000001u;
    Check(pal4::SequenceAnimPlaybackFacet::Stop(state, runtime) == 0u &&
              runtime.events.empty(), "running byte gate");

    state.words[1u] = 0x01010101u;
    Check(pal4::SequenceAnimPlaybackFacet::Stop(state, runtime) == 0x7bu &&
              state.words[1u] == 0x00000101u &&
              runtime.events == std::vector<std::string>{
                  "geometry", "clump", "world", "geometry", "clump", "remove"},
          "world removal uses two clump reads in native order");

    runtime.events.clear();
    runtime.reads = 0u;
    runtime.world = 0u;
    state.words[1u] = 0x01010101u;
    Check(pal4::SequenceAnimPlaybackFacet::Stop(state, runtime) == 0u &&
              state.words[1u] == 0x00000101u &&
              runtime.events == std::vector<std::string>{
                  "geometry", "clump", "world"},
          "missing world skips second read and removal");
}
