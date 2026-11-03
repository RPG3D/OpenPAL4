#include "pal4/particle_visual_object.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IParticleRenderStateRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> states{
        {27u, 270u}, {19u, 190u}, {20u, 200u},
        {7u, 70u}, {14u, 140u}};
    std::vector<std::string> calls;

    std::uint32_t GetState(std::uint32_t state) override {
        calls.push_back("get:" + std::to_string(state));
        return states.at(state);
    }
    int SetState(std::uint32_t state,
                 std::uint32_t value) override {
        calls.push_back("set:" + std::to_string(state) +
                        ":" + std::to_string(value));
        states[state] = value;
        return static_cast<int>(state + 1000u);
    }
    void RenderAtomic(std::uint32_t atomic) override {
        Check(atomic == 0x9000u, "render uses particle atomic");
        calls.emplace_back("render");
    }
};

void TestInactive() {
    Runtime runtime;
    pal4::ParticleVisualState state;
    Check(pal4::ParticleVisualObject::RenderState(state, runtime) == 0 &&
              runtime.calls.empty(),
          "inactive particle object leaves render state untouched");
}

void TestSaveRenderRestore() {
    Runtime runtime;
    pal4::ParticleVisualState state;
    state.WriteByte(0u, 1u);
    state.WriteByte(188u, 2u);
    state.WriteWord(192u, 5u);
    state.WriteWord(196u, 6u);
    state.WriteWord(40u, 0x9000u);
    Check(pal4::ParticleVisualObject::RenderState(state, runtime) == 1020 &&
              runtime.calls == std::vector<std::string>{
                  "get:27", "get:19", "get:20",
                  "set:27:2", "set:19:5", "set:20:6",
                  "get:7", "get:14", "set:7:1", "set:14:0",
                  "render", "set:7:70", "set:14:140",
                  "set:27:270", "set:19:190", "set:20:200"},
          "five render states save, override and restore in IDA order");
    Check(runtime.states.at(27u) == 270u &&
              runtime.states.at(19u) == 190u &&
              runtime.states.at(20u) == 200u &&
              runtime.states.at(7u) == 70u &&
              runtime.states.at(14u) == 140u,
          "render state values are fully restored");
}

}  // namespace

int main() {
    TestInactive();
    TestSaveRenderRestore();
}
