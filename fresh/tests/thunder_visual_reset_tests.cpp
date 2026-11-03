#include "pal4/thunder_visual_storage.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IThunderVisualResetRuntime {
    std::vector<std::string> calls;
    void DestroyTexture(std::uint32_t texture) override {
        calls.push_back("texture:" + std::to_string(texture));
    }
    void DestroyFrame(std::uint32_t frame) override {
        calls.push_back("frame:" + std::to_string(frame));
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::ThunderObjectState state;
    state.words.fill(0xaaaaaaaau);
    state.words[15u] &= 0xffffff00u;
    const auto original = state.words;
    pal4::ThunderVisualStorage::ResetProperties(state, runtime);
    Check(state.words == original && runtime.calls.empty(),
          "inactive visual leaves all fields and resources untouched");

    state.words[15u] |= 1u;
    state.words[29u] = 0x800u;
    state.words[34u] = 0x900u;
    state.words[28u] = 0xa00u;
    pal4::ThunderVisualStorage::ResetProperties(state, runtime);
    Check(state.words[15u] == 0x01000100u &&
              (state.words[16u] & 0xffffu) == 0u,
          "active and mode bytes reset while unrelated bytes survive");
    Check(state.words[18u] == 0x3f800000u &&
              state.words[19u] == 0x3f800000u &&
              state.words[20u] == 0x3f800000u &&
              state.words[21u] == 0u &&
              state.words[27u] == 0xffffffffu,
          "scale, rotation and sentinel defaults");
    Check(state.words[40u] == 1045220557u &&
              state.words[41u] == 0x3f000000u &&
              state.words[42u] == 0x3f000000u &&
              state.words[43u] == 0x3f800000u &&
              state.words[44u] == 1053609165u,
          "native fractional defaults");
    for (const auto index : {22u, 23u, 24u, 25u, 26u, 35u, 36u,
                             37u, 38u, 39u})
        Check(state.words[index] == 0u, "reset scalar fields");
    Check(state.words[29u] == 0u && state.words[34u] == 0u &&
              state.words[28u] == 0u &&
              state.words[0u] == 0xaaaaaaaau,
          "resource handles cleared");
    Check(runtime.calls == std::vector<std::string>{
              "texture:2048", "frame:2304", "frame:2560"},
          "texture then second frame then first frame destroyed");
    pal4::ThunderVisualStorage::ResetProperties(state, runtime);
    Check(runtime.calls.size() == 3u, "repeated reset is inactive");
}
