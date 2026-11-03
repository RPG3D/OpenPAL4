#include "pal4/dynamic_strip_object_render.h"

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

struct Runtime final : pal4::IDynamicStripBufferRuntime {
    std::vector<std::uint32_t> freed;
    void Free(std::uint32_t address) override { freed.push_back(address); }
};

void CheckCleared(const pal4::DynamicStripBufferRange& range) {
    Check(range.begin == 0u && range.end == 0u && range.capacity == 0u,
          "all three native buffer pointers are cleared");
}

}  // namespace

int main() {
    Runtime runtime;
    pal4::DynamicStripBufferRange indices{0x100u, 0x120u, 0x140u};
    pal4::DynamicStripVisualBuffers::CleanupIndices(indices, runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x100u},
          "index buffer frees only its begin pointer");
    CheckCleared(indices);

    pal4::DynamicStripBufferRange vertices{0x200u, 0x224u, 0x248u};
    pal4::DynamicStripVisualBuffers::CleanupVertices(vertices, runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x100u, 0x200u},
          "vertex buffer frees only its begin pointer");
    CheckCleared(vertices);

    pal4::DynamicStripVisualBuffers::CleanupIndices(indices, runtime);
    pal4::DynamicStripVisualBuffers::CleanupVertices(vertices, runtime);
    Check(runtime.freed.size() == 4u && runtime.freed[2] == 0u &&
              runtime.freed[3] == 0u,
          "both callbacks forward null to native free on repeated cleanup");
    CheckCleared(indices);
    CheckCleared(vertices);
}
