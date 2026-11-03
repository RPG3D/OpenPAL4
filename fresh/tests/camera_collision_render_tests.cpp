#include "pal4/camera_collision_render.h"

#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::ICameraCollisionRenderRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> states;
    pal4::CollisionRenderContext traversed{};
    std::uint32_t vertex_count = 0u;
    std::uint32_t produced_vertices = 0u;
    std::uint32_t produced_flushes = 0u;
    std::uint32_t transform_result = 1u;
    std::uint32_t statistic = 0u;
    double ftol_argument = 0.0;

    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        Check(address == 0x1004u, "camera frame offset");
        calls.emplace_back("frame");
        return 0x2000u;
    }
    [[nodiscard]] float ReadFloat(const std::uint32_t address) override {
        calls.emplace_back("float");
        if (address == 0x2030u) return 1.0f;
        if (address == 0x2034u) return 2.0f;
        if (address == 0x2038u) return 3.0f;
        Check(address == 0x1068u, "frame position or view-window offset");
        return 2.0f;
    }
    [[nodiscard]] std::int32_t Ftol(const double value) override {
        calls.emplace_back("ftol");
        ftol_argument = value;
        return static_cast<std::int32_t>(value);
    }
    [[nodiscard]] std::uint32_t SetDeviceState(
        const std::uint32_t state, const std::uint32_t value) override {
        calls.emplace_back("state");
        states.emplace_back(state, value);
        return 0u;
    }
    void SetupTransformMatrix(
        std::array<float, 16u>& matrix,
        const std::uint32_t source) override {
        Check(source == 0x2010u, "source frame matrix offset");
        calls.emplace_back("setup");
        matrix[0] = 17.0f;
    }
    void ScaleMatrix(std::array<float, 16u>& matrix,
                     const std::array<float, 3u>& scale,
                     const std::uint32_t mode) override {
        Check(matrix[0] == 17.0f &&
                  scale == std::array<float, 3u>{-0.25f, -0.25f, 0.5f} &&
                  mode == 2u, "projection scale and combine mode");
        calls.emplace_back("scale");
        matrix[1] = 18.0f;
    }
    void TranslateMatrix(std::array<float, 16u>& matrix,
                         const std::array<float, 3u>& offset,
                         const std::uint32_t mode) override {
        Check(matrix[1] == 18.0f &&
                  offset == std::array<float, 3u>{0.5f, 0.5f, 0.0f} &&
                  mode == 2u, "projection translation and combine mode");
        calls.emplace_back("translate");
        matrix[2] = 19.0f;
    }
    void TraverseIntersections(
        const std::uint32_t world,
        const std::uint32_t intersection,
        const std::uint32_t callback,
        pal4::CollisionRenderContext& context) override {
        Check(world == 0x3000u && intersection == 0x4000u &&
                  callback == 0x5fe270u, "intersection traversal arguments");
        Check(vertex_count == 0u, "global vertex count reset before traversal");
        calls.emplace_back("traverse");
        context.flush_count = produced_flushes;
        vertex_count = produced_vertices;
        traversed = context;
    }
    [[nodiscard]] std::uint32_t VertexCount() override {
        calls.emplace_back("count");
        return vertex_count;
    }
    void SetVertexCount(const std::uint32_t count) override {
        calls.emplace_back("set-count");
        vertex_count = count;
    }
    void SetStatistic(const std::uint32_t value) override {
        calls.emplace_back("statistic");
        statistic = value;
    }
    [[nodiscard]] std::uint32_t TransformVertices(
        const std::uint32_t count, const std::uint32_t matrix,
        const std::uint32_t flags) override {
        Check(count == produced_vertices && matrix == 0u && flags == 25u,
              "batched collision vertex transform arguments");
        calls.emplace_back("transform");
        return transform_result;
    }
    void RenderPrimitive(const std::uint32_t primitive) override {
        Check(primitive == 3u, "triangle strip primitive");
        calls.emplace_back("render");
    }
    void EndPrimitive() override {
        calls.emplace_back("end");
    }
    void TransformPoints(
        std::array<std::array<float, 3u>, 3u>&,
        const std::array<std::array<float, 3u>, 3u>&,
        const std::array<float, 16u>&) override {}
    [[nodiscard]] std::uint32_t GetRenderState(
        std::uint32_t) override { return 0u; }
    void SetRenderState(std::uint32_t, std::uint32_t) override {}
    void WriteVertex(std::uint32_t,
                     const pal4::CollisionImmediateVertex&) override {}
};

void CheckContext(const Runtime& runtime,
                  const std::uint8_t intensity) {
    const auto& context = runtime.traversed;
    Check(context.camera_position ==
              std::array<float, 3u>{1.0f, 2.0f, 3.0f} &&
              context.source_matrix == 0x2010u &&
              context.screen_matrix[0] == 17.0f &&
              context.screen_matrix[1] == 18.0f &&
              context.screen_matrix[2] == 19.0f,
          "collision callback position and transformed matrix");
    Check(context.intensity == intensity &&
              context.render_mode == 0x55u &&
              context.intersection == 0x4000u,
          "collision callback intensity, mode and intersection");
}

void TestEmptyAndNegativeIntensity() {
    Runtime runtime;
    Check(pal4::CameraCollisionRender::RenderWorld(
              0x1000u, 0x5000u, 0x3000u, 0x4000u,
              -0.5f, 0x9999u, 0x55u, runtime) == 1u,
          "empty render returns one");
    Check(runtime.states ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {8u, 0u}, {1u, 0x5000u}, {2u, 3u},
                  {12u, 1u}, {9u, 2u}, {10u, 1u},
                  {11u, 3u}, {8u, 1u}, {11u, 6u}, {10u, 5u}},
          "negative intensity selects blend mode three and restores state");
    Check(runtime.ftol_argument == 127.5 && runtime.statistic == 0u &&
              runtime.vertex_count == 0u,
          "absolute intensity, zero statistic and vertex count");
    CheckContext(runtime, 127u);
    Check(runtime.calls.back() == "state" &&
              runtime.calls[runtime.calls.size() - 4u] == "statistic",
          "empty batch skips immediate transform");
}

void TestBatchAndTransformFailure() {
    Runtime runtime;
    runtime.produced_vertices = 6u;
    runtime.produced_flushes = 2u;
    Check(pal4::CameraCollisionRender::RenderWorld(
              0x1000u, 0x5000u, 0x3000u, 0x4000u,
              0.5f, 0u, 0x55u, runtime) == 1u,
          "collision batch returns one");
    Check(runtime.states[6] ==
              std::pair<std::uint32_t, std::uint32_t>{11u, 4u} &&
              runtime.statistic == 1002u && runtime.vertex_count == 0u,
          "positive blend mode, statistic and final count reset");
    CheckContext(runtime, 127u);
    Check(runtime.calls[runtime.calls.size() - 6u] == "render" &&
              runtime.calls[runtime.calls.size() - 5u] == "end" &&
              runtime.calls[runtime.calls.size() - 4u] == "set-count",
          "successful immediate render and cleanup order");

    Runtime failed;
    failed.produced_vertices = 3u;
    failed.transform_result = 0u;
    Check(pal4::CameraCollisionRender::RenderWorld(
              0x1000u, 0x5000u, 0x3000u, 0x4000u,
              0.5f, 0u, 0x55u, failed) == 1u &&
              failed.vertex_count == 0u,
          "failed transform still clears count");
    Check(failed.calls[failed.calls.size() - 5u] == "transform" &&
              failed.calls[failed.calls.size() - 4u] == "set-count",
          "failed transform skips primitive render");
}

}  // namespace

int main() {
    TestEmptyAndNegativeIntensity();
    TestBatchAndTransformFailure();
}
