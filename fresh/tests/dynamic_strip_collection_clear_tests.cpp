#include "pal4/dynamic_strip_object_config.h"

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

struct Runtime final : pal4::IDynamicStripCleanupRuntime {
    std::vector<std::uint32_t> nodes{10u, 20u};
    std::vector<std::uint32_t> unlinked;
    std::vector<std::uint32_t> freed;
    std::uint32_t FirstNode(std::uint32_t sentinel) override {
        Check(sentinel == 0x700u, "list sentinel");
        return nodes.empty() ? sentinel : nodes.front();
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        Check(node == 10u || node == 20u, "known list node");
        return node == 10u ? 20u : 0x700u;
    }
    void UnlinkNode(std::uint32_t node) override { unlinked.push_back(node); }
    void FreeNode(std::uint32_t node) override { freed.push_back(node); }
    std::uint32_t DestroyTexture(std::uint32_t) override { return 0u; }
    std::uint32_t DestroyFrame(std::uint32_t) override { return 0u; }
};

}  // namespace

int main() {
    pal4::DynamicStripObjectState state;
    state.words[0u] = 0xab000001u;
    state.words[28u] = 0x700u;
    state.words[29u] = 2u;
    Runtime runtime;
    pal4::DynamicStripObjectConfiguration::ClearNonHookSegments(state, runtime);
    Check(state.words[0u] == 0xab000001u && state.words[29u] == 2u &&
              runtime.unlinked.empty(), "hook mode preserves list and bytes");

    state.words[0u] = 0x00000001u;
    pal4::DynamicStripObjectConfiguration::ClearNonHookSegments(state, runtime);
    Check(state.words[0u] == 0x00010001u && state.words[29u] == 0u,
          "ordinary mode marks capture and decrements list count");
    Check(runtime.unlinked == std::vector<std::uint32_t>{10u, 20u} &&
              runtime.freed == runtime.unlinked,
          "both nodes are unlinked and freed in traversal order");

    Runtime empty;
    empty.nodes.clear();
    pal4::DynamicStripObjectConfiguration::ClearNonHookSegments(state, empty);
    Check(state.words[29u] == 0u && empty.freed.empty(),
          "empty list leaves count unchanged");
}
