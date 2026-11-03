#include "pal4/effect_system_render_state.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, std::string_view message) {
    if (condition) return;
    std::cerr << message << '\n';
    std::exit(1);
}

struct Runtime final : pal4::IEffectSystemRenderStateRuntime {
    static constexpr std::uint32_t kSystem = 0x1000u;
    bool enabled = true;
    bool has_geometry = true;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kSystem + 4u, "render enabled address");
        calls.emplace_back("enabled");
        return enabled ? 1u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kSystem + 172u, "geometry object address");
        calls.emplace_back("geometry pointer");
        return has_geometry ? 0x7000u : 0u;
    }
    std::uint32_t GetRenderState(std::uint32_t state) override {
        calls.emplace_back("get " + std::to_string(state));
        return state * 10u;
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        calls.emplace_back("set " + std::to_string(state) + " " +
                           std::to_string(value));
    }
    void SortAtomicGeometries(std::uint32_t geometry_data) override {
        Check(geometry_data == kSystem + 52u,
              "RenderWare geometry data address");
        calls.emplace_back("sort");
    }
};

void TestRenderStateGatesAndRestore() {
    Runtime disabled;
    disabled.enabled = false;
    pal4::EffectSystem::SetRenderStates(Runtime::kSystem, disabled);
    Check(disabled.calls == std::vector<std::string>{"enabled"},
          "disabled system skips all D3D state calls");

    Runtime missing;
    missing.has_geometry = false;
    pal4::EffectSystem::SetRenderStates(Runtime::kSystem, missing);
    Check(missing.calls == (std::vector<std::string>{
              "enabled", "geometry pointer"}),
          "absent geometry object skips state changes");

    Runtime active;
    pal4::EffectSystem::SetRenderStates(Runtime::kSystem, active);
    Check(active.calls == (std::vector<std::string>{
              "enabled", "geometry pointer", "get 27", "get 19",
              "get 20", "set 27 1", "set 19 5", "set 20 6",
              "get 7", "get 14", "set 7 1", "set 14 1", "sort",
              "set 7 70", "set 14 140", "set 27 270",
              "set 19 190", "set 20 200"}),
          "temporary D3D states surround sorting in assembly order");
}

}  // namespace

int main() {
    TestRenderStateGatesAndRestore();
}
