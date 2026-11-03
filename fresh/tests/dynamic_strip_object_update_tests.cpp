#include "pal4/dynamic_strip_object_update.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IDynamicStripObjectUpdateRuntime {
    std::uint32_t calls = 0u;
    double time = 0.0;
    std::uint8_t mode = 0u;
    std::uint8_t UpdateVisual(pal4::DynamicStripObjectState& state,
                              double frame_time,
                              std::uint8_t value) override {
        Check((state.words[1u] & 0x00ff0001u) == 0x00010001u,
              "visual update sees active and enabled bytes");
        ++calls;
        time = frame_time;
        mode = value;
        return 0xa5u;
    }
};

void TestUpdate() {
    pal4::DynamicStripObjectState state{};
    Runtime runtime;
    using pal4::DynamicStripObjectUpdater;
    state.words[1u] = 0x00010000u;
    Check(DynamicStripObjectUpdater::Update(state, 0.75, runtime) == 0u &&
              runtime.calls == 0u,
          "inactive low byte skips visual update");
    state.words[1u] = 1u;
    Check(DynamicStripObjectUpdater::Update(state, 0.75, runtime) == 0u &&
              runtime.calls == 0u,
          "disabled update byte skips visual update");
    state.words[1u] = 0x00010001u;
    state.words[2u] = 0x7f000000u;
    Check(DynamicStripObjectUpdater::Update(state, 0.75, runtime) == 0xa5u &&
              runtime.calls == 1u && runtime.time == 0.75 &&
              runtime.mode == 0x7fu,
          "active update forwards time, mode and native result");
}

}  // namespace

int main() {
    TestUpdate();
}
