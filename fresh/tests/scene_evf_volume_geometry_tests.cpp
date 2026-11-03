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
        const auto address = next_address;
        next_address += 0x1000u;
        return address;
    }
    std::vector<pal4::EffectPropertyState> ReadProperties(
        std::uint32_t begin, std::uint32_t count) override {
        Check(begin == 0x4000u, "property base");
        calls.push_back("properties:" + std::to_string(count));
        std::vector<pal4::EffectPropertyState> result(count);
        for (std::uint32_t i = 0u; i < count; ++i) {
            result[i].words[0u] = i;
            result[i].words[1u] = 100u + i;
            result[i].words[2u] = 200u + i;
        }
        return result;
    }
    std::array<std::uint8_t, 4u> ReadColor(std::uint32_t address) override {
        Check(address == 0x5000u, "color pointer");
        calls.push_back("color");
        return {0x11u, 0x22u, 0x33u, 0x44u};
    }
    float PlaneFromPoints(const pal4::EffectPropertyState& first,
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
        Check(address == 0x2000u, "index buffer pointer");
        indices = value;
    }
    void WriteVertices(std::uint32_t address,
                       const std::vector<pal4::SceneEvfVertex>& value) override {
        Check(address == 0x1000u, "vertex buffer pointer");
        vertices = value;
    }
    void WriteTriangles(std::uint32_t address,
                        const std::vector<pal4::SceneEvfTriangle>& value) override {
        Check(address == 0x3000u, "plane buffer pointer");
        triangles = value;
    }
    void WarnInsufficientVertices() override { calls.push_back("warn-strip"); }
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
    state.words[22u] = 0x4000u + 80u * count;
    state.words[29u] = 0x6000u;
    state.words[31u] = 0x7000u;
    state.words[33u] = 0x5000u;
    state.words[41u] = 0x8000u;
    return state;
}

void CheckGeometry(std::uint32_t count, bool expected_result,
                   const std::vector<std::uint16_t>& expected) {
    auto state = State(count);
    Runtime runtime;
    Check(pal4::SceneEvfRecord::GenerateVolumeGeometry(state, runtime) ==
              expected_result,
          "native count comparison result");
    const auto half = count / 2u;
    const auto triangle_count = 2u * (2u * half - 4u) + count;
    const auto index_count = 3u * count + 4u * (3u * half - 6u);
    Check(state.words[32u] == count && state.words[30u] == index_count &&
              state.words[42u] == triangle_count &&
              state.words[31u] == 0x1000u && state.words[29u] == 0x2000u &&
              state.words[41u] == 0x3000u,
          "native counts and pointers");
    Check(runtime.indices == expected &&
              runtime.triangles.size() == expected.size() / 3u &&
              runtime.vertices.size() == count,
          "side and cap triangle order");
    Check(runtime.calls[0u] == "free:24576" &&
              runtime.calls[1u] == "free:28672" &&
              runtime.calls[2u] == "free:32768" &&
              runtime.calls[3u] == "alloc:" + std::to_string(36u * count) &&
              runtime.calls[4u] ==
                  "alloc:" + std::to_string(2u * index_count) &&
              runtime.calls[5u] ==
                  "alloc:" + std::to_string(28u * triangle_count),
          "release and allocation sequence");
    for (std::uint32_t i = 0u; i < count; ++i)
        Check(runtime.vertices[i].words[0u] == i &&
                  runtime.vertices[i].words[1u] == 100u + i &&
                  runtime.vertices[i].words[2u] == 200u + i &&
                  runtime.vertices[i].words[6u] == 0x44112233u,
              "position and packed color");
    for (std::size_t i = 0u; i < runtime.triangles.size(); ++i)
        Check(runtime.triangles[i].indices ==
                  (std::array<std::uint32_t, 3u>{expected[3u * i],
                                                 expected[3u * i + 1u],
                                                 expected[3u * i + 2u]}) &&
                  runtime.triangles[i].normal ==
                      (std::array<float, 3u>{
                          static_cast<float>(expected[3u * i]),
                          static_cast<float>(expected[3u * i + 1u]),
                          static_cast<float>(expected[3u * i + 2u])}) &&
                  runtime.triangles[i].distance == 42.0f,
              "plane callback follows emitted indices");
}

}  // namespace

int main() {
    CheckGeometry(8u, true, {
        0u, 4u, 1u, 4u, 5u, 1u, 1u, 5u, 2u, 5u, 6u, 2u,
        2u, 6u, 3u, 6u, 7u, 3u, 2u, 7u, 0u, 7u, 4u, 0u,
        0u, 1u, 3u, 1u, 2u, 3u, 0u, 3u, 1u, 1u, 3u, 2u,
        4u, 5u, 7u, 5u, 6u, 7u, 4u, 7u, 5u, 5u, 7u, 6u});
    CheckGeometry(9u, false, {
        0u, 4u, 1u, 4u, 5u, 1u, 1u, 5u, 2u, 5u, 6u, 2u,
        2u, 6u, 3u, 6u, 7u, 3u, 2u, 8u, 0u, 8u, 4u, 0u,
        0u, 1u, 3u, 1u, 2u, 3u, 0u, 3u, 1u, 1u, 3u, 2u,
        4u, 5u, 8u, 5u, 6u, 8u, 4u, 8u, 5u, 5u, 8u, 6u});
    CheckGeometry(10u, true, {
        0u, 5u, 1u, 5u, 6u, 1u, 1u, 6u, 2u, 6u, 7u, 2u,
        2u, 7u, 3u, 7u, 8u, 3u, 3u, 8u, 4u, 8u, 9u, 4u,
        3u, 9u, 0u, 9u, 5u, 0u,
        0u, 1u, 4u, 1u, 2u, 4u, 4u, 2u, 3u,
        0u, 4u, 1u, 1u, 4u, 2u, 4u, 3u, 2u,
        5u, 6u, 9u, 6u, 7u, 9u, 9u, 7u, 8u,
        5u, 9u, 6u, 6u, 9u, 7u, 9u, 8u, 7u});

    for (std::uint32_t count = 0u; count < 8u; ++count) {
        auto state = State(count);
        state.words[30u] = 77u;
        state.words[42u] = 88u;
        Runtime runtime;
        Check(!pal4::SceneEvfRecord::GenerateVolumeGeometry(state, runtime) &&
                  state.words[32u] == count && state.words[30u] == 77u &&
                  state.words[42u] == 88u &&
                  state.words[22u] == state.words[21u] &&
                  runtime.calls == std::vector<std::string>{
                      "free:24576", "free:28672", "free:32768",
                      "warn-volume",
                      "clear:16384:" + std::to_string(16384u + 80u * count)},
              "short input does not overrun native buffers");
    }
}
