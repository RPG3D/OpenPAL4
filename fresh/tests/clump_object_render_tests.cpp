#include "pal4/clump_object_render.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IClumpObjectRenderRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> states{
        {27u, 12u}, {19u, 13u}, {20u, 14u},
        {7u, 15u}, {14u, 16u}};
    std::uint32_t stage_value = 17u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1014u, "clump pointer is manager +20");
        calls.push_back("read");
        return 0x2000u;
    }
    std::uint32_t GetRenderState(std::uint32_t state) override {
        calls.push_back("get:" + std::to_string(state));
        return states.at(state);
    }
    std::uint8_t SetRenderState(std::uint32_t state,
                                std::uint32_t value) override {
        calls.push_back("set:" + std::to_string(state) + ":" +
                        std::to_string(value));
        states[state] = value;
        return state == 20u && value == 14u ? 0x7bu : 0x21u;
    }
    std::uint32_t GetTextureStageState(std::uint32_t stage) override {
        Check(stage == 20u, "texture stage 20 getter");
        calls.push_back("stage:get");
        return stage_value;
    }
    void SetTextureStageState(std::uint32_t stage,
                              std::uint32_t value) override {
        Check(stage == 20u, "texture stage 20 setter");
        calls.push_back("stage:set:" + std::to_string(value));
        stage_value = value;
    }
    void Render(std::uint32_t clump) override {
        Check(clump == 0x2000u && states[27u] == 1u &&
                  states[19u] == 5u && states[20u] == 6u &&
                  states[7u] == 1u && states[14u] == 1u &&
                  stage_value == 2u,
              "clump renders with temporary D3D9 state");
        calls.push_back("render");
    }
};

void TestRender() {
    pal4::ClumpObjectState state{};
    state.words[18u] = 0x1000u;
    Runtime runtime;
    Check(pal4::ClumpObject::RenderClump(state, 0x99u, runtime) == 0u &&
              runtime.calls.empty(), "inactive object returns byte +4");
    state.words[1u] = 1u;
    Check(pal4::ClumpObject::RenderClump(state, 0x99u, runtime) == 0u &&
              runtime.calls.empty(), "disabled rendering returns byte +7");
    state.words[1u] = 0x56000001u;
    Check(pal4::ClumpObject::RenderClump(state, 0x99u, runtime) == 0x7bu,
          "render returns final native state-set result");
    Check(runtime.calls == std::vector<std::string>{
              "read", "get:27", "get:19", "get:20", "set:27:1",
              "set:19:5", "set:20:6", "get:7", "get:14",
              "set:7:1", "set:14:1", "stage:get", "stage:set:2",
              "render", "stage:set:17", "set:7:15", "set:14:16",
              "set:27:12", "set:19:13", "set:20:14"} &&
              runtime.states == std::map<std::uint32_t, std::uint32_t>{
                  {27u, 12u}, {19u, 13u}, {20u, 14u},
                  {7u, 15u}, {14u, 16u}} &&
              runtime.stage_value == 17u,
          "render preserves and restores all native states in order");
}

}  // namespace

int main() {
    TestRender();
}
