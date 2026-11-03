#include "pal4/scene_evf_record.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool good, const char* message) {
    if (!good) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISceneEvfGeometryRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint16_t> indices;
    std::vector<pal4::SceneEvfVertex> vertices;
    std::vector<pal4::SceneEvfTriangle> triangles;
    std::uint32_t next_address = 0x1000u;
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("alloc:" + std::to_string(bytes));
        const auto result = next_address;
        next_address += 0x1000u;
        return result;
    }
    std::vector<pal4::EffectPropertyState> ReadProperties(
        std::uint32_t begin, std::uint32_t count) override {
        Check(begin == 0x4000u, "geometry reads native property begin");
        calls.push_back("properties:" + std::to_string(count));
        std::vector<pal4::EffectPropertyState> result(count);
        for (std::uint32_t i = 0u; i < count; ++i) {
            result[i].words[0u] = i;
            result[i].words[1u] = 100u + i;
            result[i].words[2u] = 200u + i;
        }
        return result;
    }
    std::array<std::uint8_t, 4u> ReadColor(
        std::uint32_t address) override {
        Check(address == 0x5000u, "geometry reads color pointer");
        calls.push_back("color");
        return {0x11u, 0x22u, 0x33u, 0x44u};
    }
    float PlaneFromPoints(
        const pal4::EffectPropertyState& first,
        const pal4::EffectPropertyState& second,
        const pal4::EffectPropertyState& third,
        std::array<float, 3u>& normal) override {
        normal = {static_cast<float>(first.words[0u]),
                  static_cast<float>(second.words[0u]),
                  static_cast<float>(third.words[0u])};
        calls.push_back("plane");
        return 42.0f;
    }
    void WriteIndices(std::uint32_t address,
                      const std::vector<std::uint16_t>& value) override {
        Check(address == 0x1000u, "indices written to first allocation");
        calls.push_back("indices");
        indices = value;
    }
    void WriteVertices(std::uint32_t address,
                       const std::vector<pal4::SceneEvfVertex>& value) override {
        Check(address == 0x2000u, "vertices written to second allocation");
        calls.push_back("vertices");
        vertices = value;
    }
    void WriteTriangles(
        std::uint32_t address,
        const std::vector<pal4::SceneEvfTriangle>& value) override {
        Check(address == 0x3000u, "planes written to third allocation");
        calls.push_back("triangles");
        triangles = value;
    }
    void WarnInsufficientVertices() override {
        calls.push_back("warn");
    }
    void WarnInsufficientPolygonVertices() override {
        calls.push_back("warn-polygon");
    }
    void WarnInsufficientVolumeVertices() override {
        calls.push_back("warn-volume");
    }
    void ClearPropertyRange(std::uint32_t begin,
                            std::uint32_t end) override {
        calls.push_back("clear:" + std::to_string(begin) + ":" +
                        std::to_string(end));
    }
};

pal4::SceneEvfRecordState State(std::uint32_t count) {
    pal4::SceneEvfRecordState state;
    state.words[21u] = 0x4000u;
    state.words[22u] = 0x4000u + count * 80u;
    state.words[33u] = 0x5000u;
    state.words[29u] = 0x6000u;
    state.words[31u] = 0x7000u;
    state.words[41u] = 0x8000u;
    return state;
}

void CheckGeometry(bool both_sides, bool close,
                   const std::vector<std::uint16_t>& expected) {
    auto state = State(4u);
    Runtime runtime;
    Check(pal4::SceneEvfRecord::GenerateStripGeometry(
              state, both_sides, close, runtime),
          "four-vertex strip generates geometry");
    Check(state.words[32u] == 4u &&
              state.words[30u] == expected.size() &&
              state.words[42u] == expected.size() / 3u &&
              state.words[29u] == 0x1000u &&
              state.words[31u] == 0x2000u &&
              state.words[41u] == 0x3000u,
          "geometry counts and allocated pointers");
    Check(runtime.indices == expected &&
              runtime.triangles.size() == expected.size() / 3u &&
              runtime.vertices.size() == 4u,
          "native triangle topology and vertex count");
    for (std::size_t i = 0u; i < runtime.vertices.size(); ++i)
        Check(runtime.vertices[i].words[0u] == i &&
                  runtime.vertices[i].words[1u] == 100u + i &&
                  runtime.vertices[i].words[2u] == 200u + i &&
                  runtime.vertices[i].words[6u] == 0x44112233u,
              "vertex copies three coordinates and packed ARGB");
    for (std::size_t i = 0u; i < runtime.triangles.size(); ++i)
        Check(runtime.triangles[i].indices[0u] == expected[3u * i] &&
                  runtime.triangles[i].indices[1u] == expected[3u * i + 1u] &&
                  runtime.triangles[i].indices[2u] == expected[3u * i + 2u] &&
                  runtime.triangles[i].normal ==
                      std::array<float, 3u>{
                          static_cast<float>(expected[3u * i]),
                          static_cast<float>(expected[3u * i + 1u]),
                          static_cast<float>(expected[3u * i + 2u])} &&
                  runtime.triangles[i].distance == 42.0f,
              "plane callback receives native vertex indices");
    Check(runtime.calls[0u] == "free:24576" &&
              runtime.calls[1u] == "free:28672" &&
              runtime.calls[2u] == "free:32768" &&
              runtime.calls[3u] ==
                  "alloc:" + std::to_string(2u * expected.size()) &&
              runtime.calls[4u] == "alloc:144" &&
              runtime.calls[5u] ==
                  "alloc:" + std::to_string(
                      28u * expected.size() / 3u),
          "old buffers released and new sizes allocated in native order");
}

}  // namespace

int main() {
    CheckGeometry(false, false, {0u, 2u, 1u, 2u, 3u, 1u});
    CheckGeometry(false, true,
                  {0u, 2u, 1u, 2u, 3u, 1u,
                   1u, 3u, 0u, 3u, 2u, 0u});
    CheckGeometry(true, false,
                  {0u, 2u, 1u, 2u, 3u, 1u,
                   1u, 3u, 0u, 3u, 2u, 0u});
    CheckGeometry(true, true,
                  {0u, 2u, 1u, 2u, 3u, 1u,
                   1u, 3u, 0u, 3u, 2u, 0u,
                   1u, 3u, 0u, 3u, 2u, 0u,
                   0u, 2u, 1u, 2u, 3u, 1u});

    auto six = State(6u);
    Runtime six_runtime;
    Check(pal4::SceneEvfRecord::GenerateStripGeometry(
              six, true, false, six_runtime) &&
              six.words[30u] == 24u && six.words[42u] == 8u &&
              six_runtime.indices == std::vector<std::uint16_t>{
                  0u, 3u, 1u, 3u, 4u, 1u,
                  1u, 4u, 2u, 4u, 5u, 2u,
                  2u, 5u, 1u, 5u, 4u, 1u,
                  1u, 4u, 0u, 4u, 3u, 0u},
          "six-vertex two-sided strip iterates all paired rows");

    auto two = State(2u);
    Runtime two_runtime;
    Check(pal4::SceneEvfRecord::GenerateStripGeometry(
              two, false, false, two_runtime) &&
              two.words[30u] == 0u && two.words[42u] == 0u &&
              two_runtime.vertices.size() == 2u &&
              two_runtime.indices.empty() &&
              two_runtime.triangles.empty() &&
              two_runtime.calls[3u] == "alloc:0" &&
              two_runtime.calls[4u] == "alloc:72" &&
              two_runtime.calls[5u] == "alloc:0",
          "two-vertex open strip allocates vertices with zero triangles");

    auto short_state = State(1u);
    short_state.words[30u] = 77u;
    short_state.words[42u] = 88u;
    Runtime short_runtime;
    Check(!pal4::SceneEvfRecord::GenerateStripGeometry(
              short_state, false, false, short_runtime) &&
              short_state.words[32u] == 1u &&
              short_state.words[22u] == short_state.words[21u] &&
              short_state.words[30u] == 77u &&
              short_state.words[42u] == 88u &&
              short_runtime.calls == std::vector<std::string>{
                  "free:24576", "free:28672", "free:32768",
                  "warn", "clear:16384:16464"},
          "short strip warns, clears property range and keeps old counts");
}
