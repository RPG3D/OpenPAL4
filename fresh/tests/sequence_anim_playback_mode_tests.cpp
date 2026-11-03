#include "pal4/sequence_anim_playback_mode.h"

#include <bit>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISequenceAnimPlaybackModeRuntime {
    std::uint32_t queries = 0u;
    std::uint32_t durations = 0u;
    std::int32_t frame = 30;
    double duration = 60.0;
    void QueryPlayback(std::uint32_t facet_vtable,
                       std::uint32_t facet_offset,
                       std::int32_t& first,
                       std::int32_t& second,
                       std::int32_t& third,
                       std::int32_t& output) override {
        Check(facet_vtable == 0x84a888u && facet_offset == 36u,
              "playback facet and offset");
        Check(first == 0 && second == 0 && third == 0 && output == 0,
              "playback query zero arguments");
        ++queries;
        output = frame;
    }
    double Duration(const pal4::SequenceAnimObjectState& state) override {
        Check(state.words[2u] == 0x01020304u,
              "duration query after mode-byte write");
        ++durations;
        return duration;
    }
};

}  // namespace

int main() {
    pal4::SequenceAnimObjectState state{};
    state.words[2u] = 0x00020304u;
    state.words[9u] = 0x84a888u;
    state.words[29u] = 0xdeadbeefu;
    Runtime runtime;

    pal4::SequenceAnimPlaybackMode::Set(state, 1, runtime);
    Check(state.words[2u] == 0x00020304u && runtime.queries == 0u,
          "inactive owner gate");
    state.words[1u] = 0x100u;
    pal4::SequenceAnimPlaybackMode::Set(state, 1, runtime);
    Check(runtime.queries == 0u, "activity uses low byte");
    state.words[1u] = 1u;
    pal4::SequenceAnimPlaybackMode::Set(state, 0, runtime);
    Check(runtime.queries == 0u && state.words[29u] == 0xdeadbeefu,
          "unchanged zero mode skips query");
    pal4::SequenceAnimPlaybackMode::Set(state, 1, runtime);
    Check(state.words[2u] == 0x01020304u &&
              std::bit_cast<float>(state.words[29u]) == 0.5f &&
              runtime.queries == 1u && runtime.durations == 1u,
          "enabled mode queries playback and stores ratio");
    pal4::SequenceAnimPlaybackMode::Set(state, 0x101, runtime);
    Check(runtime.queries == 1u, "mode comparison uses low byte");
    pal4::SequenceAnimPlaybackMode::Set(state, 0, runtime);
    Check(state.words[2u] == 0x00020304u && runtime.queries == 1u &&
              state.words[29u] == std::bit_cast<std::uint32_t>(0.5f),
          "disabling preserves cached ratio");
}
