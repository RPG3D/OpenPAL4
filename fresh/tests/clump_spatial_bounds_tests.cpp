#include "pal4/clump_spatial_bounds.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(const double lhs, const double rhs) {
    return std::fabs(lhs - rhs) < 0.0001;
}

struct Runtime final : pal4::IClumpSpatialBoundsRuntime {
    std::vector<std::string> events;
    pal4::ClumpBoundsVector camera_position{};
    std::array<pal4::ClumpBoundsVertex, 8> rendered_vertices{};
    bool transform_succeeds = true;

    void SetupMatrix(std::array<float, 16>& output,
                     const std::uint32_t source) override {
        Check(source == 0x3000u, "uses matrix setup input");
        output[0] = 1.0f;
        events.emplace_back("setup");
    }
    void MultiplyMatrix(std::array<float, 16>& output,
                        const std::uint32_t lhs,
                        const std::array<float, 16>& rhs) override {
        Check(lhs == 0x2000u && rhs[0] == 1.0f,
              "multiplies source and prepared matrices");
        output[0] = output[5] = output[10] = 1.0f;
        events.emplace_back("multiply");
    }
    [[nodiscard]] std::uint32_t AllocatePoints(
        const std::uint32_t bytes, const std::uint32_t hint) override {
        Check(bytes == 36u && hint == 0u,
              "allocates twelve bytes per input vertex");
        events.emplace_back("allocate");
        return 0x4000u;
    }
    void TransformPoints(const std::uint32_t destination,
                         const std::uint32_t source,
                         const std::uint32_t count,
                         const std::array<float, 16>& matrix) override {
        Check(destination == 0x4000u && source == 0x1000u && count == 3u &&
                  matrix[5] == 1.0f,
              "transforms points into allocated buffer");
        events.emplace_back("points");
    }
    void CalculateBox(pal4::ClumpBoundsVector& maximum,
                      pal4::ClumpBoundsVector& minimum,
                      const std::uint32_t points,
                      const std::uint32_t count) override {
        Check(points == 0x4000u && count == 3u,
              "calculates box from transformed points");
        maximum = {3.0f, 4.0f, 5.0f};
        minimum = {-1.0f, -2.0f, -3.0f};
        events.emplace_back("box");
    }
    void TransformVector(pal4::ClumpBoundsVector& destination,
                         const pal4::ClumpBoundsVector& source,
                         const std::array<float, 16>&) override {
        const auto copy = source;
        destination = copy;
        events.emplace_back("vector");
    }
    [[nodiscard]] std::uint32_t FreePoints(
        const std::uint32_t points) override {
        Check(points == 0x4000u, "frees transformed point buffer");
        events.emplace_back("free");
        return 0xabcu;
    }
    void CalculateCameraPosition(
        pal4::ClumpBoundsVector& output, const std::uint32_t camera,
        const std::array<float, 16>& matrix) override {
        Check(camera == 0x5000u && matrix[0] == 1.0f,
              "uses camera and clump matrix");
        output = camera_position;
        events.emplace_back("camera");
    }
    [[nodiscard]] int SetRenderState(const int state,
                                     const int value) override {
        events.emplace_back("state:" + std::to_string(state) + ":" +
                            std::to_string(value));
        return state == 7 && value == 2 ? 42 : 0;
    }
    [[nodiscard]] bool TransformImmediate(
        const std::array<pal4::ClumpBoundsVertex, 8>& vertices,
        const std::uint32_t matrix, const int flags) override {
        Check(matrix == 0x6000u && flags == 26,
              "immediate transform uses caller matrix and native flags");
        rendered_vertices = vertices;
        events.emplace_back("immediate-transform");
        return transform_succeeds;
    }
    void RenderIndexedLines(const int primitive,
                            const int index_count) override {
        Check(primitive == 1 && index_count == 24,
              "renders twelve indexed bounding-box edges");
        events.emplace_back("render-lines");
    }
    void EndImmediate() override { events.emplace_back("end-immediate"); }
};

}  // namespace

int main() {
    using Bounds = pal4::ClumpSpatialBounds;
    pal4::ClumpBoundsState state;
    Runtime runtime;
    Check(Bounds::CalculateBoundingBox(
              state, 0x1000u, 3u, 0x2000u, 0x3000u, runtime) == 0xabcu &&
              runtime.events == std::vector<std::string>({
                  "setup", "multiply", "allocate", "points", "box",
                  "vector", "free"}) &&
              Near(state.center.x, 1.0) && Near(state.center.y, 1.0) &&
              Near(state.center.z, 1.0) &&
              Near(state.half_extent.x, 2.0) &&
              Near(state.half_extent.y, 3.0) &&
              Near(state.half_extent.z, 4.0) &&
              Near(state.transformed_center.x, 1.0),
          "bounding-box setup derives center and half extents in native order");

    state.transformed_center = {10.0f, 20.0f, 30.0f};
    pal4::ClumpBoundsVector closest;
    Check(Near(Bounds::GetIntersectionDistance(
                   state, {13.0f, 22.0f, 30.0f}, &closest), 1.0) &&
              Near(closest.x, 12.0) && Near(closest.y, 22.0) &&
              Near(closest.z, 30.0),
          "outside point returns squared excess distance and closest point");
    Check(Near(Bounds::GetIntersectionDistance(
                   state, {10.0f, 20.0f, 30.0f}, nullptr), 0.0) &&
              Near(Bounds::GetIntersectionDistance(
                   state, {12.003f, 20.0f, 30.0f}, nullptr), 0.0) &&
              Bounds::GetIntersectionDistance(
                  state, {12.02f, 20.0f, 30.0f}, nullptr) > 0.0,
          "inside point and native 0.005 face tolerance produce zero distance");
    runtime.events.clear();
    runtime.camera_position = {10.0f, 20.0f, 30.0f};
    Check(Bounds::CheckIntersection(state, 0x5000u, &closest, runtime) &&
              runtime.events == std::vector<std::string>({
                  "camera", "vector"}) && Near(closest.x, 10.0),
          "camera check transforms position and returns intersection");
    runtime.camera_position = {20.0f, 20.0f, 30.0f};
    Check(!Bounds::CheckIntersection(state, 0x5000u, nullptr, runtime),
          "outside camera position fails intersection");

    runtime.events.clear();
    Check(Bounds::RenderBoundingBox(state, 0x11223344u, 0x6000u,
                                    runtime) == 42 &&
              runtime.events == std::vector<std::string>({
                  "state:7:1", "state:1:0", "immediate-transform",
                  "render-lines", "end-immediate", "state:7:2"}) &&
              Near(runtime.rendered_vertices[0].position.x, -1.0) &&
              Near(runtime.rendered_vertices[7].position.x, 3.0) &&
              Near(runtime.rendered_vertices[7].position.y, 4.0) &&
              Near(runtime.rendered_vertices[7].position.z, 5.0) &&
              runtime.rendered_vertices[0].color == 0x11443322u,
          "render fills eight native corners and restores state");
    runtime.events.clear();
    runtime.transform_succeeds = false;
    Check(Bounds::RenderBoundingBox(state, 0u, 0x6000u, runtime) == 42 &&
              runtime.events == std::vector<std::string>({
                  "state:7:1", "state:1:0", "immediate-transform",
                  "state:7:2"}),
          "failed immediate transform skips draw and still restores state");
    return EXIT_SUCCESS;
}
