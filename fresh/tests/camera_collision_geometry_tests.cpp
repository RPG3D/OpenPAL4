#include "pal4/camera_collision_render.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

using Points = std::array<std::array<float, 3u>, 3u>;

struct Runtime final : pal4::ICameraCollisionRenderRuntime {
    std::map<std::uint32_t, float> floats;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::CollisionImmediateVertex> vertices;
    Points projected{{
        {0.1f, 0.2f, 0.3f},
        {0.5f, 0.6f, 0.7f},
        {0.9f, 0.8f, 0.9f}}};
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> state_writes;
    std::vector<double> ftol_arguments;
    std::uint32_t vertex_count = 0u;
    std::uint32_t transform_result = 1u;
    std::uint32_t saved_state = 0x55u;

    Runtime() {
        floats[0x3000u] = -1.0f;
        floats[0x3004u] = 0.0f;
        floats[0x3008u] = 0.0f;
        words[0x301cu] = 0x4000u;
        words[0x3020u] = 0x4100u;
        words[0x3024u] = 0x4200u;
        floats[0x4000u] = 1.0f;
        floats[0x4004u] = 2.0f;
        floats[0x4008u] = 3.0f;
        floats[0x4100u] = 4.0f;
        floats[0x4104u] = 5.0f;
        floats[0x4108u] = 6.0f;
        floats[0x4200u] = 7.0f;
        floats[0x4204u] = 8.0f;
        floats[0x4208u] = 9.0f;
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        Check(words.contains(address), "triangle point-pointer offset");
        calls.emplace_back("word");
        return words[address];
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        Check(floats.contains(address), "triangle normal or point offset");
        calls.emplace_back("float");
        return floats[address];
    }
    [[nodiscard]] std::int32_t Ftol(const double value) override {
        calls.emplace_back("ftol");
        ftol_arguments.push_back(value);
        return static_cast<std::int32_t>(value);
    }
    [[nodiscard]] std::uint32_t SetDeviceState(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void SetupTransformMatrix(std::array<float, 16u>&,
                              std::uint32_t) override {}
    void ScaleMatrix(std::array<float, 16u>&,
                     const std::array<float, 3u>&,
                     std::uint32_t) override {}
    void TranslateMatrix(std::array<float, 16u>&,
                         const std::array<float, 3u>&,
                         std::uint32_t) override {}
    void TraverseIntersections(std::uint32_t, std::uint32_t,
                               std::uint32_t,
                               pal4::CollisionRenderContext&) override {}
    [[nodiscard]] std::uint32_t VertexCount() override {
        calls.emplace_back("count");
        return vertex_count;
    }
    void SetVertexCount(const std::uint32_t count) override {
        calls.emplace_back("set-count");
        vertex_count = count;
    }
    void SetStatistic(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t TransformVertices(
        const std::uint32_t count, const std::uint32_t matrix,
        const std::uint32_t flags) override {
        Check(count == 1498u && matrix == 0u && flags == 25u,
              "overflow flush transform arguments");
        calls.emplace_back("transform-vertices");
        return transform_result;
    }
    void RenderPrimitive(const std::uint32_t primitive) override {
        Check(primitive == 3u, "overflow flush primitive");
        calls.emplace_back("render");
    }
    void EndPrimitive() override { calls.emplace_back("end"); }
    void TransformPoints(
        Points& destination, const Points& source,
        const std::array<float, 16u>& matrix) override {
        Check(source == Points{{{1.0f, 2.0f, 3.0f},
                                {4.0f, 5.0f, 6.0f},
                                {7.0f, 8.0f, 9.0f}}} &&
                  matrix[0] == 17.0f,
              "point order and callback matrix");
        calls.emplace_back("project");
        destination = projected;
    }
    [[nodiscard]] std::uint32_t GetRenderState(
        const std::uint32_t state) override {
        Check(state == 28u, "D3D render-state index");
        calls.emplace_back("get-state");
        return saved_state;
    }
    void SetRenderState(const std::uint32_t state,
                        const std::uint32_t value) override {
        Check(state == 28u, "D3D render-state write index");
        calls.emplace_back("set-state");
        state_writes.emplace_back(state, value);
    }
    void WriteVertex(const std::uint32_t index,
                     const pal4::CollisionImmediateVertex& vertex) override {
        calls.emplace_back("vertex");
        vertices[index] = vertex;
    }
};

pal4::CollisionRenderContext Context(const std::uint32_t mode = 0u) {
    pal4::CollisionRenderContext context{};
    context.camera_position = {1.0f, 0.0f, 0.0f};
    context.screen_matrix[0] = 17.0f;
    context.intensity = 100u;
    context.render_mode = mode;
    return context;
}

std::uint32_t Render(Runtime& runtime,
                     pal4::CollisionRenderContext& context) {
    return pal4::CameraCollisionRender::RenderGeometry(
        0x6000u, 0x7000u, 0x3000u, 0.25f, context, runtime);
}

void TestFacingAndViewportCulling() {
    Runtime backface;
    backface.floats[0x3000u] = 1.0f;
    auto context = Context();
    Check(Render(backface, context) == 0x3000u &&
              backface.calls == std::vector<std::string>{
                  "float", "float", "float"},
          "positive normal dot returns triangle before point reads");

    for (const auto& points : std::vector<Points>{
             Points{{{0.1f, 0.2f, -0.1f},
                     {0.5f, 0.6f, -0.2f},
                     {0.9f, 0.8f, -0.3f}}},
             Points{{{-0.1f, 0.2f, 0.1f},
                     {-0.2f, 0.6f, 0.2f},
                     {-0.3f, 0.8f, 0.3f}}},
             Points{{{1.1f, 0.2f, 0.1f},
                     {1.2f, 0.6f, 0.2f},
                     {1.3f, 0.8f, 0.3f}}},
             Points{{{0.1f, -0.1f, 0.1f},
                     {0.5f, -0.2f, 0.2f},
                     {0.9f, -0.3f, 0.3f}}},
             Points{{{0.1f, 1.1f, 0.1f},
                     {0.5f, 1.2f, 0.2f},
                     {0.9f, 1.3f, 0.3f}}}}) {
        Runtime culled;
        culled.projected = points;
        auto culled_context = Context();
        Check(Render(culled, culled_context) == 0x3000u &&
                  culled.calls.back() == "project" &&
                  culled.vertices.empty() &&
                  culled.state_writes.empty(),
              "each out-of-viewport plane returns before D3D state calls");
    }
}

void TestVisibleTriangleAndConstantColor() {
    Runtime runtime;
    auto context = Context();
    Check(Render(runtime, context) == 0x3000u &&
              runtime.vertex_count == 3u &&
              context.flush_count == 0u,
          "visible triangle appends three vertices");
    Check(runtime.state_writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {28u, 0u}, {28u, 0x55u}},
          "D3D state 28 is disabled then restored");
    Check(runtime.ftol_arguments.empty() && runtime.vertices.size() == 3u,
          "constant color avoids CRT conversion");
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        const auto& vertex = runtime.vertices.at(i);
        Check(vertex.x == 1.0f + 3.0f * i &&
                  vertex.y == 2.0f + 3.0f * i + 0.4f &&
                  vertex.z == 3.0f + 3.0f * i &&
                  vertex.u == runtime.projected[i][0] &&
                  vertex.v == runtime.projected[i][1] &&
                  vertex.color == 0x64646464u &&
                  vertex.native_tail ==
                      std::array<std::uint32_t, 3u>{0u, 0u, 0u},
              "36-byte vertex position, projected UV, color and defined tail");
    }
}

void TestProjectedColorAndBufferFlush() {
    Runtime runtime;
    runtime.projected = {{{0.1f, 0.3f, 0.0f},
                          {0.5f, 0.7f, 0.5f},
                          {0.9f, 0.9f, 2.0f}}};
    auto context = Context(1u);
    Check(Render(runtime, context) == 0x3000u &&
              runtime.ftol_arguments == std::vector<double>{100.0, 75.0} &&
              runtime.vertices.at(0u).color == 0x64646464u &&
              runtime.vertices.at(1u).color == 0x4b4b4b4bu &&
              runtime.vertices.at(2u).color == 0u,
          "projected Z squared scales color and negative factors become zero");

    Runtime full;
    full.vertex_count = 1498u;
    auto full_context = Context();
    Check(Render(full, full_context) == 0x3000u &&
              full_context.flush_count == 1u &&
              full.vertex_count == 3u &&
              full.vertices.contains(0u) && full.vertices.contains(2u),
          "buffer over threshold flushes and restarts at vertex zero");
    Check(full.calls[full.calls.size() - 3u] == "vertex" &&
              full.calls[full.calls.size() - 2u] == "set-count" &&
              full.calls.back() == "set-state",
          "vertex count update precedes D3D state restoration");
    Check(std::find(full.calls.begin(), full.calls.end(), "render") !=
              full.calls.end(), "successful overflow transform renders");

    Runtime failed;
    failed.vertex_count = 1498u;
    failed.transform_result = 0u;
    auto failed_context = Context();
    Check(Render(failed, failed_context) == 0x3000u &&
              failed_context.flush_count == 1u &&
              failed.vertex_count == 3u &&
              std::find(failed.calls.begin(), failed.calls.end(), "render") ==
                  failed.calls.end(),
          "failed overflow transform still records flush and appends vertices");

    Runtime exact;
    exact.vertex_count = 1497u;
    auto exact_context = Context();
    Check(Render(exact, exact_context) == 0x3000u &&
              exact.vertex_count == 1500u &&
              exact_context.flush_count == 0u &&
              exact.vertices.contains(1497u) &&
              std::find(exact.calls.begin(), exact.calls.end(),
                        "transform-vertices") == exact.calls.end(),
          "1497 vertices accept one more triangle without early flush");
}

}  // namespace

int main() {
    TestFacingAndViewportCulling();
    TestVisibleTriangleAndConstantColor();
    TestProjectedColorAndBufferFlush();
}
