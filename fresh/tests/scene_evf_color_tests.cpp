#include "pal4/scene_evf_record.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISceneEvfColorRuntime {
    std::vector<std::uint32_t> colors;
    void WriteVertexColor(std::uint32_t address, std::uint32_t index,
                          std::uint32_t argb) override {
        Check(address == 0x7000u && index == colors.size(),
              "36-byte vertex buffer traversal");
        colors.push_back(argb);
    }
};

}  // namespace

int main() {
    pal4::SceneEvfRecordState state;
    state.words[31u] = 0x7000u;
    state.words[34u] = 0x44332211u;
    state.words[35u] = 0xddccbbaau;
    Runtime runtime;
    Check(pal4::SceneEvfRecord::UpdateVertexColors(
              state, 0x4000u, false, runtime) == 0u &&
              state.words[33u] == 0x4088u && runtime.colors.empty(),
          "zero vertices still selects primary inline color");

    state.words[32u] = 3u;
    Check(pal4::SceneEvfRecord::UpdateVertexColors(
              state, 0x4000u, true, runtime) == 3u &&
              state.words[33u] == 0x408cu &&
              runtime.colors == std::vector<std::uint32_t>{
                  0xddaabbccu, 0xddaabbccu, 0xddaabbccu},
          "alternate RGBA color is packed into every vertex");

    runtime.colors.clear();
    Check(pal4::SceneEvfRecord::UpdateVertexColors(
              state, 0x4000u, false, runtime) == 3u &&
              state.words[33u] == 0x4088u &&
              runtime.colors == std::vector<std::uint32_t>{
                  0x44112233u, 0x44112233u, 0x44112233u},
          "primary color replaces previous vertex colors");
}
