#include "pal4/scene_evf_record.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISceneEvfIntersectionRuntime {
    std::vector<std::string> calls;
    std::vector<std::array<std::uint32_t, 3u>> indices{
        {0u, 1u, 2u}, {3u, 4u, 5u}, {6u, 7u, 8u}};
    std::array<bool, 3u> hits{true, true, true};
    std::array<float, 3u> hit_x{4.0f, 7.0f, 5.0f};

    void CalculatePixelRay(std::uint32_t camera, std::uint32_t pixel,
                           pal4::SceneEvfVec3& start,
                           pal4::SceneEvfVec3& end) override {
        Check(camera == 0x100u && pixel == 0x200u, "camera and pixel args");
        calls.push_back("ray");
        start = {1.0f, 2.0f, 3.0f};
        end = {2.0f, 2.0f, 3.0f};
    }
    void TransformCameraRay(std::uint32_t camera,
                            pal4::SceneEvfVec3& start,
                            pal4::SceneEvfVec3& end) override {
        Check(camera == 0x100u && start[0u] == 1.0f && end[0u] == 2.0f,
              "transform receives camera ray");
        calls.push_back("transform-ray");
        start[0u] = 9.0f;
        end[0u] = 10.0f;
    }
    std::array<std::uint32_t, 3u> ReadTriangleIndices(
        std::uint32_t address, std::uint32_t index) override {
        Check(address == 0x7000u, "triangle buffer address");
        calls.push_back("triangle:" + std::to_string(index));
        return indices[index];
    }
    pal4::SceneEvfVec3 ReadPropertyPosition(
        std::uint32_t address, std::uint32_t index) override {
        Check(address == 0x4000u, "property buffer address");
        calls.push_back("position:" + std::to_string(index));
        return {static_cast<float>(index), 0.0f, 0.0f};
    }
    void TransformTriangle(
        std::uint32_t camera,
        std::array<pal4::SceneEvfVec3, 3u>& points) override {
        Check(camera == 0x100u, "triangle transform camera");
        calls.push_back("transform-triangle");
        for (auto& point : points) point[0u] += 10.0f;
    }
    bool IntersectLine(
        const pal4::SceneEvfVec3& start,
        const pal4::SceneEvfVec3& end,
        const std::array<pal4::SceneEvfVec3, 3u>& points,
        pal4::SceneEvfVec3& hit) override {
        Check(start[0u] == 9.0f && end[0u] == 10.0f,
              "intersection receives transformed ray");
        const auto triangle = static_cast<std::uint32_t>(points[0u][0u]) / 3u - 3u;
        Check(points[1u][0u] == points[0u][0u] + 1.0f &&
                  points[2u][0u] == points[0u][0u] + 2.0f,
              "three property points transformed together");
        calls.push_back("intersect:" + std::to_string(triangle));
        hit = {hit_x[triangle], 2.0f, 3.0f};
        return hits[triangle];
    }
    float Length(const pal4::SceneEvfVec3& vector) override {
        calls.push_back("length");
        return std::sqrt(vector[0u] * vector[0u] +
                         vector[1u] * vector[1u] +
                         vector[2u] * vector[2u]);
    }
};

}  // namespace

int main() {
    pal4::SceneEvfRecordState state;
    state.words[21u] = 0x4000u;
    state.words[41u] = 0x7000u;
    Runtime runtime;
    float distance = 0.0f;
    Check(!pal4::SceneEvfRecord::IntersectsPixel(
              state, 0x100u, 0x200u, &distance, runtime) &&
              distance == std::numeric_limits<float>::max() &&
              runtime.calls == std::vector<std::string>{
                  "ray", "transform-ray"},
          "empty geometry still builds ray and writes maximum distance");

    state.words[42u] = 3u;
    runtime.calls.clear();
    Check(pal4::SceneEvfRecord::IntersectsPixel(
              state, 0x100u, 0x200u, &distance, runtime) &&
              distance == 2.0f &&
              runtime.calls.size() == 2u + 3u * 7u &&
              runtime.calls[2u] == "triangle:0" &&
              runtime.calls[9u] == "triangle:1" &&
              runtime.calls[16u] == "triangle:2",
          "nearest of three intersections wins after all triangles");

    runtime.hits = {false, false, false};
    runtime.calls.clear();
    Check(!pal4::SceneEvfRecord::IntersectsPixel(
              state, 0x100u, 0x200u, &distance, runtime) &&
              distance == std::numeric_limits<float>::max() &&
              runtime.calls.size() == 2u + 3u * 6u,
          "misses skip length and retain maximum distance");

    runtime.hits = {false, true, false};
    Check(pal4::SceneEvfRecord::IntersectsPixel(
              state, 0x100u, 0x200u, nullptr, runtime),
          "null distance output is allowed");
}
