#include "pal4/pal_actor_control_renderer.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlRenderRuntime {
    pal4::PalActorControlState* state = nullptr;
    std::vector<std::string> calls;
    bool first_transform = true;
    bool second_transform = true;
    std::vector<pal4::ActorControlCircularEffect> effects;

    void SetRenderState(std::uint32_t id,
                        std::uint32_t value) override {
        calls.emplace_back("state " + std::to_string(id) + " " +
                           std::to_string(value));
    }
    bool TransformLine(const std::uint32_t* vertices,
                       std::uint32_t count,
                       std::uint32_t flags) override {
        const auto offset = vertices - state->words.data();
        Check((offset == 225 || offset == 243) &&
                  count == 2u && flags == 2u,
              "two observed vertex spans and transform flags");
        calls.emplace_back("transform " + std::to_string(offset));
        return offset == 225 ? first_transform : second_transform;
    }
    void RenderIndexedPrimitive(
        std::uint32_t kind,
        const std::array<std::uint16_t, 2>& indices) override {
        Check(kind == 1u && indices[0] == 0u && indices[1] == 1u,
              "line primitive and original indices");
        calls.emplace_back("indexed");
    }
    void End3D() override { calls.emplace_back("end"); }
    double ActorDoubleProperty(std::uint32_t actor) override {
        Check(actor == 0x7000u, "actor property address");
        calls.emplace_back("property");
        return 8.0;
    }
    pal4::ActorControlVector3 ActorPosition(
        std::uint32_t actor) override {
        Check(actor == 0x7000u, "actor position address");
        calls.emplace_back("position");
        return {10.0f, 20.0f, 30.0f};
    }
    void RenderCircularEffect(
        const pal4::ActorControlCircularEffect& effect) override {
        calls.emplace_back("circle");
        effects.push_back(effect);
    }
};

void TestGates() {
    for (const auto zero_index : {0u, 6u, 1u}) {
        pal4::PalActorControlState state{};
        state.words[0] = 1u;
        state.words[6] = 1u;
        state.words[1] = 0x7000u;
        state.words[zero_index] = 0u;
        pal4::ActorControlRenderGlobals globals{};
        globals.upper.radius = 99.0f;
        Runtime runtime;
        runtime.state = &state;
        pal4::PalActorControlRenderer::Draw(state, globals, runtime);
        Check(runtime.calls.empty() && globals.upper.radius == 99.0f,
              "each render gate stops all calls and global writes");
    }
}

void TestRenderBranches() {
    for (const bool first : {false, true}) {
        for (const bool second : {false, true}) {
            pal4::PalActorControlState state{};
            state.words[0] = 1u;
            state.words[6] = 1u;
            state.words[1] = 0x7000u;
            state.words[9] = std::bit_cast<std::uint32_t>(3.5f);
            Runtime runtime;
            runtime.state = &state;
            runtime.first_transform = first;
            runtime.second_transform = second;
            pal4::ActorControlRenderGlobals globals{};
            pal4::PalActorControlRenderer::Draw(state, globals, runtime);
            std::vector<std::string> expected{
                "state 7 1", "state 6 0", "state 1 0",
                "transform 225"};
            if (first) {
                expected.emplace_back("indexed");
                expected.emplace_back("end");
            }
            expected.emplace_back("transform 243");
            if (second) {
                expected.emplace_back("indexed");
                expected.emplace_back("end");
            }
            const std::vector<std::string> tail{
                "state 6 1", "state 7 2", "property", "position",
                "circle", "circle"};
            expected.insert(expected.end(), tail.begin(), tail.end());
            Check(runtime.calls == expected,
                  "RenderWare outcomes preserve state and circle order");
            Check(runtime.effects.size() == 2u &&
                      globals.upper.position.x == 10.0f &&
                      globals.upper.position.y == 25.0f &&
                      globals.upper.position.z == 30.0f &&
                      globals.upper.radius == 3.5f &&
                      globals.lower.position.x == 10.0f &&
                      globals.lower.position.y == 23.0f &&
                      globals.lower.position.z == 30.0f &&
                      globals.lower.radius == 3.5f &&
                      runtime.effects[0].position.y == 25.0f &&
                      runtime.effects[1].position.y == 23.0f,
                  "two circle globals use half-height and offsets");
        }
    }
}

void TestLinkableStub() {
    pal4::PalActorControlState state{};
    state.words[0] = 1u;
    state.words[1] = 1u;
    state.words[6] = 1u;
    pal4::ActorControlRenderGlobals globals{};
    pal4::ActorControlRenderRuntimeStub runtime;
    pal4::PalActorControlRenderer::Draw(state, globals, runtime);
    Check(globals.upper.position.y == 0.0f &&
              globals.lower.position.y == 0.0f,
          "RenderWare-free stub links and runs both circle paths");
}

}  // namespace

int main() {
    TestGates();
    TestRenderBranches();
    TestLinkableStub();
}
