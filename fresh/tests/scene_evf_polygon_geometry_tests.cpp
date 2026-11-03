#include "pal4/scene_evf_record.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
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
        Check(address == 0x5000u, "color address");
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
        Check(address == 0x2000u, "index allocation address");
        calls.push_back("indices");
        indices = value;
    }
    void WriteVertices(std::uint32_t address,
                       const std::vector<pal4::SceneEvfVertex>& value) override {
        Check(address == 0x1000u, "vertex allocation address");
        calls.push_back("vertices");
        vertices = value;
    }
    void WriteTriangles(std::uint32_t address,
                        const std::vector<pal4::SceneEvfTriangle>& value) override {
        Check(address == 0x3000u, "plane allocation address");
        calls.push_back("triangles");
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

void CheckPolygon(std::uint32_t count,
                  const std::vector<std::uint16_t>& expected) {
    auto state = State(count);
    Runtime runtime;
    Check(pal4::SceneEvfRecord::GeneratePolygonGeometry(state, runtime),
          "polygon generation succeeds");
    Check(state.words[32u] == count && state.words[30u] == expected.size() &&
              state.words[42u] == expected.size() / 3u &&
              state.words[31u] == 0x1000u && state.words[29u] == 0x2000u &&
              state.words[41u] == 0x3000u,
          "native counts and pointers");
    Check(runtime.indices == expected && runtime.vertices.size() == count &&
              runtime.triangles.size() == expected.size() / 3u,
          "two-sided polygon topology");
    Check(runtime.calls[0u] == "free:24576" &&
              runtime.calls[1u] == "free:28672" &&
              runtime.calls[2u] == "free:32768" &&
              runtime.calls[3u] == "alloc:" + std::to_string(36u * count) &&
              runtime.calls[4u] == "alloc:" +
                  std::to_string(2u * expected.size()) &&
              runtime.calls[5u] == "alloc:" +
                  std::to_string(28u * expected.size() / 3u),
          "native release and allocation order");
    for (std::uint32_t i = 0u; i < count; ++i)
        Check(runtime.vertices[i].words[0u] == i &&
                  runtime.vertices[i].words[1u] == 100u + i &&
                  runtime.vertices[i].words[2u] == 200u + i &&
                  runtime.vertices[i].words[6u] == 0x44112233u,
              "position and ARGB payload");
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
              "plane callback and record order");
}

}  // namespace

int main() {
    CheckPolygon(4u, {0u, 1u, 3u, 1u, 2u, 3u,
                      0u, 3u, 1u, 1u, 3u, 2u});
    CheckPolygon(5u, {0u, 1u, 4u, 1u, 2u, 4u, 4u, 2u, 3u,
                      0u, 4u, 1u, 1u, 4u, 2u, 4u, 3u, 2u});
    CheckPolygon(6u, {0u, 1u, 5u, 1u, 2u, 5u, 5u, 2u, 4u, 2u, 3u, 4u,
                      0u, 5u, 1u, 1u, 5u, 2u, 5u, 4u, 2u, 2u, 4u, 3u});

    for (std::uint32_t count = 0u; count <= 3u; ++count) {
        auto state = State(count);
        state.words[30u] = 77u;
        state.words[42u] = 88u;
        Runtime runtime;
        Check(!pal4::SceneEvfRecord::GeneratePolygonGeometry(state, runtime) &&
                  state.words[32u] == count && state.words[30u] == 77u &&
                  state.words[42u] == 88u &&
                  state.words[22u] == state.words[21u] &&
                  runtime.calls == std::vector<std::string>{
                      "free:24576", "free:28672", "free:32768",
                      "warn-polygon",
                      "clear:16384:" + std::to_string(16384u + 80u * count)},
              "short polygon rejects unsafe input and clears records");
    }
}
