#include "pal4/cegui_immediate_rectangle.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::CeguiImmediateRectangleStub {
public:
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> set;
    std::array<pal4::CeguiRenderVertex, 4> vertices{};
    std::uint32_t texture = 0u;
    float ActiveCameraDepth() override { return 2.0f; }
    float RenderDepth() override { return 0.25f; }
    std::uint32_t GetRenderState(const std::uint32_t id) override {
        calls.push_back("get");
        return id + 100u;
    }
    std::uint32_t SetRenderState(const std::uint32_t id,
                                 const std::uint32_t value) override {
        calls.push_back("set");
        set.emplace_back(id, value);
        return id + value;
    }
    void BindTexture(const std::uint32_t handle) override {
        calls.push_back("bind");
        texture = handle;
    }
    std::uint32_t DrawTriangleStrip(
        const std::array<pal4::CeguiRenderVertex, 4>& input) override {
        calls.push_back("draw");
        vertices = input;
        return 0xabcdu;
    }
};

pal4::CeguiRectWords Words(const float a, const float b,
                           const float c, const float d) {
    return {std::bit_cast<std::uint32_t>(a),
            std::bit_cast<std::uint32_t>(b),
            std::bit_cast<std::uint32_t>(c),
            std::bit_cast<std::uint32_t>(d)};
}

void GeometryAndColourOrder() {
    Runtime runtime;
    const std::array<pal4::CeguiColourWords, 4> colours{{
        {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
    }};
    const auto result = pal4::CeguiImmediateRectangle::Draw(
        Words(10.0f, 20.0f, 30.0f, 40.0f), 0x1234u,
        Words(0.1f, 0.2f, 0.8f, 0.9f), colours, runtime);
    Check(result == 104u && runtime.texture == 0x1234u,
          "draw returns the last state restoration result and binds texture");
    const auto& v = runtime.vertices;
    Check(v[0].x == 29.5f && v[0].y == 19.5f &&
              v[1].x == 39.5f && v[1].y == 19.5f &&
              v[2].x == 29.5f && v[2].y == 9.5f &&
              v[3].x == 39.5f && v[3].y == 9.5f,
          "native Rect top/bottom/left/right produces LB/RB/LT/RT strip");
    Check(v[0].u == 0.8f && v[0].v == 0.2f &&
              v[1].u == 0.9f && v[1].v == 0.2f &&
              v[2].u == 0.8f && v[2].v == 0.1f &&
              v[3].u == 0.9f && v[3].v == 0.1f,
          "UV words follow the original copied rectangle slots");
    Check(v[0].colour == 0xffff0000u &&
              v[1].colour == 0xff0000ffu &&
              v[2].colour == 0xff000000u &&
              v[3].colour == 0xff00ff00u,
          "colours are packed and selected in original vertex order");
    for (const auto& vertex : v) {
        Check(vertex.depth == 0.25f &&
                  vertex.reciprocal_camera_depth == 0.5f,
              "all vertices carry render depth and camera reciprocal");
    }
}

void RenderStateOrder() {
    Runtime runtime;
    const std::array<pal4::CeguiColourWords, 4> colours{};
    static_cast<void>(pal4::CeguiImmediateRectangle::Draw(
        Words(0, 0, 1, 1), 1u, Words(0, 0, 1, 1), colours, runtime));
    const std::vector<std::string> expected{
        "get", "get", "get", "get", "get", "get",
        "set", "set", "set", "set", "set", "set",
        "bind", "draw", "set", "set", "set", "set", "set", "set"};
    Check(runtime.calls == expected,
          "six render states are saved, set, then restored after drawing");
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> expected_states{
        {12u, 1u}, {6u, 0u}, {8u, 0u}, {7u, 2u}, {9u, 1u}, {2u, 3u},
        {12u, 112u}, {6u, 106u}, {8u, 108u},
        {7u, 107u}, {9u, 109u}, {2u, 102u}};
    Check(runtime.set == expected_states,
          "state values and restoration order match IDA");
}

void ScaledVariantUsesRendererDimensionsAndCommonStates() {
    Runtime runtime;
    pal4::CeguiRendererState state;
    state.WriteFloat(272u, 2.0f);
    state.WriteFloat(276u, 3.0f);
    const std::array<pal4::CeguiColourWords, 4> colours{};
    const auto result = pal4::CeguiImmediateRectangle::DrawScaled(
        state, Words(10, 20, 30, 40), 0x5678u,
        Words(0.1f, 0.2f, 0.8f, 0.9f), colours, runtime);
    const auto& v = runtime.vertices;
    Check(result == 104u && runtime.texture == 0x5678u &&
              v[0].x == 59.5f && v[0].y == 59.5f &&
              v[1].x == 79.5f && v[1].y == 59.5f &&
              v[2].x == 59.5f && v[2].y == 29.5f &&
              v[3].x == 79.5f && v[3].y == 29.5f,
          "scaled draw applies both renderer factors to every corner");
    Check(runtime.set[4] ==
              std::pair<std::uint32_t, std::uint32_t>{9u, 2u} &&
              v[0].u == 0.8f && v[0].v == 0.2f,
          "scaled draw uses common state nine and unchanged UVs");
}

void OffsetVariantReturnsDrawResultWithoutRestoration() {
    Runtime runtime;
    pal4::CeguiRendererState state;
    state.WriteFloat(272u, 2.0f);
    state.WriteFloat(276u, 3.0f);
    state.WriteFloat(280u, 11.0f);
    const std::array<pal4::CeguiColourWords, 4> colours{};
    const auto result = pal4::CeguiImmediateRectangle::DrawOffset(
        state, Words(10, 20, 30, 40), 0x4321u,
        Words(0.1f, 0.2f, 0.8f, 0.9f), colours, runtime);
    const auto& v = runtime.vertices;
    Check(result == 0xabcdu && runtime.texture == 0x4321u &&
              v[0].x == 71.0f && v[0].y == 60.0f &&
              v[1].x == 91.0f && v[1].y == 60.0f &&
              v[2].x == 71.0f && v[2].y == 30.0f &&
              v[3].x == 91.0f && v[3].y == 30.0f,
          "offset draw applies horizontal bias and no half-pixel subtraction");
    Check(runtime.calls == std::vector<std::string>{
              "set", "set", "set", "set", "set", "set",
              "bind", "draw"} &&
              runtime.set == std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {12u, 1u}, {6u, 0u}, {8u, 0u},
                  {7u, 2u}, {9u, 2u}, {2u, 3u}},
          "offset draw uses common states without save or restore");
    Check(v[0].u == 0.8f && v[0].v == 0.2f &&
              v[2].u == 0.8f && v[2].v == 0.1f &&
              v[0].colour == 0u,
          "offset variant keeps UV slots and zero channel colour");
}

void EmptySlotHasNoEffects() {
    pal4::CeguiImmediateRectangle::EmptyRendererSlot();
    Check(true, "empty renderer slot is callable");
}

}  // namespace

int main() {
    GeometryAndColourOrder();
    RenderStateOrder();
    ScaledVariantUsesRendererDimensionsAndCommonStates();
    OffsetVariantReturnsDrawResultWithoutRestoration();
    EmptySlotHasNoEffects();
    std::cout << "5 CEGUI immediate rectangle test cases passed\n";
}
