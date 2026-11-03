#include "pal4/thunder_visual_path.h"

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

struct Runtime final : pal4::IThunderSplineBufferRuntime {
    std::vector<std::uint32_t> freed;
    void Free(std::uint32_t address) override {
        freed.push_back(address);
    }
};

void CheckEmpty(const pal4::ThunderSplineBufferRange& range) {
    Check(range.begin == 0u && range.end == 0u &&
              range.capacity == 0u,
          "all three buffer pointers cleared");
}

}  // namespace

int main() {
    Runtime runtime;
    pal4::ThunderSplineBufferRange second{0x200u, 0x224u, 0x248u};
    pal4::ThunderSplineBufferRange first{0x100u, 0x124u, 0x148u};
    pal4::ThunderSplineBuffers::CleanupSecond(second, runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x200u},
          "second spline control buffer frees its begin pointer");
    CheckEmpty(second);
    pal4::ThunderSplineBuffers::CleanupFirst(first, runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x200u, 0x100u},
          "first spline control buffer frees its begin pointer");
    CheckEmpty(first);
    pal4::ThunderSplineBuffers::CleanupSecond(second, runtime);
    pal4::ThunderSplineBuffers::CleanupFirst(first, runtime);
    Check(runtime.freed.size() == 4u && runtime.freed[2] == 0u &&
              runtime.freed[3] == 0u,
          "repeated cleanup forwards null to native free");
    CheckEmpty(first);
    CheckEmpty(second);
}
