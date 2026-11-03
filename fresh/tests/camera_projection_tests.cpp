#include "pal4/camera_projection.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraScreenProjectionRuntime {
public:
    std::vector<std::string> calls;
    std::uint32_t raster = 0x2000u;
    std::uint32_t type = 1u;
    std::array<float, 3> position{2.0f, 3.0f, 5.0f};

    std::uint32_t ColorRaster(std::uint32_t camera) override {
        Check(camera == 0x1000u, "projection camera receiver");
        calls.emplace_back("raster");
        return raster;
    }
    std::pair<std::int32_t, std::int32_t> RasterSize(
        std::uint32_t handle) override {
        Check(handle == 0x2000u, "projection raster size receiver");
        calls.emplace_back("size");
        return {640, 480};
    }
    std::array<float, 3> CalculatePosition(
        std::uint32_t source, std::uint32_t camera,
        std::uint32_t transform_offset) override {
        Check(source == 0x3000u && camera == 0x1000u &&
                  transform_offset == 0x20u,
              "projection position operands");
        calls.emplace_back("position");
        return position;
    }
    float NearPlane(std::uint32_t camera) override {
        Check(camera == 0x1000u, "near plane receiver");
        calls.emplace_back("near");
        return 1.0f;
    }
    float FarPlane(std::uint32_t camera) override {
        Check(camera == 0x1000u, "far plane receiver");
        calls.emplace_back("far");
        return 10.0f;
    }
    std::uint32_t CameraType(std::uint32_t camera) override {
        Check(camera == 0x1000u, "camera projection type receiver");
        calls.emplace_back("type");
        return type;
    }
    float ViewWindowX(std::uint32_t camera) override {
        Check(camera == 0x1000u, "camera view-window receiver");
        calls.emplace_back("view X");
        return 0.5f;
    }
};

void ProjectCases() {
    Runtime runtime;
    pal4::CameraProjection projection(runtime);
    pal4::CameraDataState state;
    std::array<float, 2> output{};
    constexpr auto missing = std::numeric_limits<float>::max();
    Check(&projection.Project(state, output, 0x3000u) == &output &&
              output == std::array<float, 2>{missing, missing} &&
              runtime.calls.empty(),
          "absent camera returns output sentinel without runtime calls");

    state.At(0) = 0x1000u;
    runtime.raster = 0u;
    Check(&projection.Project(state, output, 0x3000u) == &output &&
              output == std::array<float, 2>{missing, missing} &&
              runtime.calls == std::vector<std::string>{"raster"},
          "absent color raster returns sentinel");

    runtime.calls.clear();
    runtime.raster = 0x2000u;
    Check(&projection.Project(state, output, 0x3000u) == &output &&
              output == std::array<float, 2>{256.0f, 288.0f} &&
              runtime.calls == std::vector<std::string>{
                  "raster", "position", "near", "far",
                  "type", "view X", "size"},
          "perspective projection uses depth, view X and raster dimensions");

    runtime.calls.clear();
    runtime.type = 0u;
    Check(&projection.Project(state, output, 0x3000u) == &output &&
              output == std::array<float, 2>{2.0f, 3.0f} &&
              runtime.calls == std::vector<std::string>{
                  "raster", "position", "near", "far", "type"},
          "non-perspective projection returns calculated XY directly");

    runtime.calls.clear();
    runtime.position[2] = 0.5f;
    projection.Project(state, output, 0x3000u);
    Check(output == std::array<float, 2>{missing, missing} &&
              runtime.calls == std::vector<std::string>{
                  "raster", "position", "near"},
          "near-plane rejection skips far and camera type reads");

    runtime.calls.clear();
    runtime.position[2] = 11.0f;
    projection.Project(state, output, 0x3000u);
    Check(output == std::array<float, 2>{missing, missing} &&
              runtime.calls == std::vector<std::string>{
                  "raster", "position", "near", "far"},
          "far-plane rejection keeps output sentinel");

    runtime.position[2] = 1.0f;
    projection.Project(state, output, 0x3000u);
    Check(output == std::array<float, 2>{2.0f, 3.0f},
          "near-plane equality remains visible");
    runtime.position[2] = 10.0f;
    projection.Project(state, output, 0x3000u);
    Check(output == std::array<float, 2>{2.0f, 3.0f},
          "far-plane equality remains visible");
    runtime.position[2] = std::numeric_limits<float>::quiet_NaN();
    projection.Project(state, output, 0x3000u);
    Check(output == std::array<float, 2>{2.0f, 3.0f},
          "unordered x87 comparisons preserve non-perspective XY");
}

void UnprojectCases() {
    Runtime runtime;
    pal4::CameraProjection projection(runtime);
    pal4::CameraDataState state;
    std::array<float, 3> output{};
    const std::array<float, 2> screen{160.0f, 120.0f};
    constexpr auto missing = std::numeric_limits<float>::max();
    Check(&projection.Unproject(state, output, screen, 5.0f) == &output &&
              output == std::array<float, 3>{missing, missing, missing} &&
              runtime.calls.empty(),
          "unprojection without camera returns three sentinels");

    state.At(0) = 0x1000u;
    runtime.raster = 0u;
    projection.Unproject(state, output, screen, 5.0f);
    Check(output == std::array<float, 3>{missing, missing, missing} &&
              runtime.calls == std::vector<std::string>{"raster"},
          "unprojection without raster returns sentinels");

    runtime.calls.clear();
    runtime.raster = 0x2000u;
    Check(&projection.Unproject(state, output, screen, 5.0f) == &output &&
              output == std::array<float, 3>{1.25f, 1.25f, 5.0f} &&
              runtime.calls == std::vector<std::string>{
                  "raster", "size", "type", "view X"},
          "perspective unprojection uses centered pixel offset and depth");

    runtime.calls.clear();
    runtime.type = 0u;
    Check(&projection.Unproject(state, output, screen, 5.0f) == &output &&
              output == std::array<float, 3>{160.0f, 120.0f, 5.0f} &&
              runtime.calls == std::vector<std::string>{
                  "raster", "size", "type"},
          "non-perspective unprojection returns centered XY and depth");
}

}  // namespace

int main() {
    try {
        ProjectCases();
        UnprojectCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
