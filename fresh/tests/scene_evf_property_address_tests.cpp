#include "pal4/scene_evf_record.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::SceneEvfRecordState state;
    state.words[22u] = 0x5000u;
    Check(pal4::SceneEvfRecord::PropertyAddress(state, 0u) == 0u,
          "null base returns null");
    state.words[21u] = 0x4000u;
    state.words[22u] = 0x4000u + 3u * 80u + 79u;
    Check(pal4::SceneEvfRecord::PropertyAddress(state, 0u) == 0x4000u &&
              pal4::SceneEvfRecord::PropertyAddress(state, 1u) == 0x4050u &&
              pal4::SceneEvfRecord::PropertyAddress(state, 2u) == 0x40a0u,
          "in-range address uses 80-byte stride");
    Check(pal4::SceneEvfRecord::PropertyAddress(state, 3u) == 0u &&
              pal4::SceneEvfRecord::PropertyAddress(state, 0xffffffffu) == 0u,
          "end and high index return null");
    state.words[22u] = state.words[21u];
    Check(pal4::SceneEvfRecord::PropertyAddress(state, 0u) == 0u,
          "empty range returns null");
}
