#include "pal4/scene_evf_record.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISceneEvfPropertyBufferRuntime {
    std::map<std::uint32_t, pal4::EffectPropertyState> properties;
    std::vector<std::uint32_t> freed;
    pal4::EffectPropertyState& PropertyAt(std::uint32_t address) override {
        return properties.at(address);
    }
    void FreePropertyBuffer(std::uint32_t address) override {
        freed.push_back(address);
    }
};

}  // namespace

int main() {
    Runtime range_runtime;
    range_runtime.properties[0x5000u].words[16u] = 9u;
    range_runtime.properties[0x5050u].words[17u] = 8u;
    range_runtime.properties[0x5050u].words[19u] = 7u;
    pal4::SceneEvfRecord::ClearPropertyRange(
        0x5000u, 0x50a0u, range_runtime);
    Check(range_runtime.properties[0x5000u].words[16u] == 0u &&
              range_runtime.properties[0x5050u].words[17u] == 0u &&
              range_runtime.properties[0x5050u].words[19u] == 0u &&
              range_runtime.freed.empty(),
          "range clear visits two records without freeing storage");
    pal4::SceneEvfRecord::ClearPropertyRange(
        0x5000u, 0x5000u, range_runtime);

    pal4::SceneEvfRecordState state;
    Runtime runtime;
    state.words[21u] = 0x4000u;
    state.words[22u] = 0x40a0u;
    state.words[23u] = 0x40f0u;
    runtime.properties[0x4000u].words[16u] = 1u;
    runtime.properties[0x4050u].words[17u] = 2u;
    runtime.properties[0x4050u].words[19u] = 3u;
    runtime.properties[0x4050u].words[0u] = 4u;
    Check(pal4::SceneEvfRecord::PropertyBufferSize(state) == 2,
          "active property range has two records");
    Check(pal4::SceneEvfRecord::CleanupPropertyBuffer(state, runtime) == 0 &&
              runtime.freed == std::vector<std::uint32_t>{0x4000u} &&
              state.words[21u] == 0u && state.words[22u] == 0u &&
              state.words[23u] == 0u &&
              runtime.properties[0x4000u].words[16u] == 0u &&
              runtime.properties[0x4050u].words[17u] == 0u &&
              runtime.properties[0x4050u].words[19u] == 0u &&
              runtime.properties[0x4050u].words[0u] == 4u,
          "every record clears before the backing buffer is freed");

    pal4::SceneEvfRecordState empty;
    Runtime empty_runtime;
    Check(pal4::SceneEvfRecord::CleanupPropertyBuffer(
              empty, empty_runtime) == 0 &&
              empty_runtime.freed == std::vector<std::uint32_t>{0u},
          "empty buffer still calls free with null");
    empty.words[22u] = 0x4000u;
    Check(pal4::SceneEvfRecord::PropertyBufferSize(empty) == 0,
          "null base returns zero regardless of end");
    empty.words[21u] = 0x4000u;
    empty.words[22u] = 0x3fb0u;
    Check(pal4::SceneEvfRecord::PropertyBufferSize(empty) == -1,
          "native signed division handles reversed range");
}
