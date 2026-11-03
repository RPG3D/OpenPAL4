#include "pal4/pal_actor_control_circle_renderer.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool Near(float actual, float expected) {
    return std::abs(actual - expected) < 0.0001f;
}

struct Runtime : pal4::IActorControlCircleRuntime {
    std::vector<std::string> events;
    std::vector<float> angles;
    std::vector<pal4::ActorControlCircleVertex> vertices;
    std::uint32_t transform_result{};

    void SinCos(float angle, float& sine, float& cosine) override {
        angles.push_back(angle);
        IActorControlCircleRuntime::SinCos(angle, sine, cosine);
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        events.emplace_back("state " + std::to_string(state) + ":" +
                            std::to_string(value));
    }
    std::uint32_t Transform(
        const pal4::ActorControlCircleVertex* first, std::uint32_t count,
        std::uint32_t flags) override {
        Check(count == 21u && flags == 2u,
              "transform vertex count and flags");
        vertices.assign(first, first + count);
        events.emplace_back("transform");
        return transform_result;
    }
    void RenderPrimitive(std::uint32_t kind) override {
        Check(kind == 2u, "line strip primitive kind");
        events.emplace_back("render");
    }
    std::uint32_t End3D() override {
        events.emplace_back("end");
        return 0x1234u;
    }
};

}  // namespace

int main() {
    pal4::ActorControlCircularEffect effect{};
    effect.position = {10.0f, 20.0f, 30.0f};
    effect.radius = 2.0f;
    Runtime runtime;
    Check(pal4::PalActorControlCircleRenderer::Draw(effect, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "state 6:1", "state 1:0", "transform"} &&
              runtime.angles.size() == 21u &&
              Near(runtime.angles.front(), 0.0f) &&
              Near(runtime.angles.back(), 6.2831853f) &&
              runtime.vertices.size() == 21u,
          "failed transform skips render and end after 21 samples");
    for (const auto& vertex : runtime.vertices) {
        Check(vertex.color == pal4::PalActorControlCircleRenderer::kColor &&
                  Near(vertex.y, 20.0f),
              "each vertex receives color and center height");
    }
    Check(Near(runtime.vertices[0].x, 12.0f) &&
              Near(runtime.vertices[0].z, 30.0f) &&
              Near(runtime.vertices[5].x, 10.0f) &&
              Near(runtime.vertices[5].z, 32.0f) &&
              Near(runtime.vertices[10].x, 8.0f) &&
              Near(runtime.vertices[10].z, 30.0f) &&
              Near(runtime.vertices[20].x, 12.0f) &&
              Near(runtime.vertices[20].z, 30.0f),
          "circle samples close at 2 pi and keep the center offset");

    runtime = {};
    runtime.transform_result = 7u;
    Check(pal4::PalActorControlCircleRenderer::Draw(effect, runtime) ==
              0x1234u &&
              runtime.events == std::vector<std::string>{
                  "state 6:1", "state 1:0", "transform", "render", "end"},
          "successful transform renders and returns end result");
}
