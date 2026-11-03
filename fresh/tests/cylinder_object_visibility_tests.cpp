#include "pal4/cylinder_object_visibility.h"

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

struct Runtime final : pal4::ICylinderObjectVisibilityRuntime {
    std::vector<std::string> calls;
    std::uint32_t world = 0x3000u;
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x2004u, "clump at model +4");
        calls.push_back("read_clump");
        return 0x4000u;
    }
    std::uint32_t FindWorld(std::uint32_t clump) override {
        Check(clump == 0x4000u, "world lookup clump");
        calls.push_back("find_world");
        return world;
    }
    std::uint8_t RemoveClump(std::uint32_t value,
                             std::uint32_t clump) override {
        Check(value == 0x3000u && clump == 0x4000u,
              "world removal arguments");
        calls.push_back("remove_clump");
        return 0xa5u;
    }
};

void TestDeactivate() {
    pal4::CylinderObjectState state{};
    state.words[1u] = 0x11010100u;
    state.words[22u] = 0x2000u;
    Runtime runtime;
    using pal4::CylinderObjectVisibilityFacet;
    Check(CylinderObjectVisibilityFacet::Deactivate(
              state, runtime) == 0u &&
              state.words[1u] == 0x11010100u && runtime.calls.empty(),
          "inactive low byte does not clear visibility flags");
    state.words[1u] |= 1u;
    Check(CylinderObjectVisibilityFacet::Deactivate(
              state, runtime) == 0xa5u &&
              state.words[1u] == 0x00000101u &&
              runtime.calls == std::vector<std::string>{
                  "read_clump", "find_world", "read_clump",
                  "remove_clump"},
          "active facet clears flags and removes world clump");
    runtime.calls.clear();
    runtime.world = 0u;
    state.words[1u] = 0x01010101u;
    Check(CylinderObjectVisibilityFacet::Deactivate(
              state, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read_clump", "find_world"} &&
              state.words[1u] == 0x00000101u,
          "absent world skips second clump read and removal");
}

}  // namespace

int main() {
    TestDeactivate();
}
