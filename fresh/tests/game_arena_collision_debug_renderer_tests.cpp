#include "pal4/game_arena_collision_debug_renderer.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Renderer = pal4::GameArenaCollisionDebugRenderer;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string Event(char operation, std::uint32_t value) {
    return std::string(1, operation) + std::to_string(value);
}

struct Runtime final : pal4::IGameArenaCollisionDebugRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5004u, 0x6000u}, {0x5008u, 0x6004u},
        {0x6000u, 0x9000u},
        {0x9100u, 1u}, {0x9104u, 2u}, {0x9108u, 3u},
        {0x9120u, 4u}, {0x9124u, 5u}, {0x9128u, 6u},
        {0x9140u, 7u}, {0x9144u, 8u}, {0x9148u, 9u},
        {0x9200u, 11u}, {0x9204u, 12u}, {0x9208u, 13u},
        {0x9220u, 14u}, {0x9224u, 15u}, {0x9228u, 16u},
        {0x9240u, 17u}, {0x9244u, 18u}, {0x9248u, 19u}};
    std::map<std::uint32_t, std::uint32_t> rendered;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> stateChanges;
    std::vector<std::string> events;
    std::uint32_t buffer = 0xa000u;
    std::uint32_t transformResult = 1u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back(Event('R', address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back(Event('W', address));
        writes.emplace_back(address, value);
        rendered[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size,
                           std::uint32_t flags) override {
        events.push_back(Event('A', size));
        Check(flags == 0u, "allocator flags are zero");
        return buffer;
    }
    std::uint32_t GetVertexAddress(std::uint32_t triangle,
                                   std::uint32_t index) override {
        events.push_back(Event('G', index));
        Check(index < 3u, "vertex index is one of three");
        return (triangle == 0x9000u ? 0x9100u : 0x9200u) + index * 0x20u;
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        events.push_back(Event('S', state));
        stateChanges.emplace_back(state, value);
    }
    std::uint32_t Transform(std::uint32_t vertices,
                            std::uint32_t count, std::uint32_t matrix,
                            std::uint32_t flags) override {
        events.push_back(Event('T', count));
        Check(vertices == buffer && matrix == 0u && flags == 0u,
              "Im3D transform arguments");
        return transformResult;
    }
    void RenderPrimitive(std::uint32_t type) override {
        events.push_back(Event('P', type));
    }
    void EndTransform() override { events.push_back("E"); }
    void Free(std::uint32_t address) override {
        events.push_back(Event('F', address));
    }
};

void NoTriangles() {
    Runtime runtime;
    runtime.words[0x5004u] = 0u;
    Renderer::Render(0x5000u, runtime);
    Check(runtime.events == std::vector<std::string>{Event('R', 0x5004u)},
          "null list stops before reading end pointer");

    runtime = Runtime{};
    runtime.words[0x5008u] = 0x6000u;
    Renderer::Render(0x5000u, runtime);
    Check(runtime.events == (std::vector<std::string>{
              Event('R', 0x5004u), Event('R', 0x5008u)}) &&
              runtime.writes.empty(),
          "empty list allocates and renders nothing");

    runtime = Runtime{};
    runtime.words[0x5008u] = 0x5ffcu;
    Renderer::Render(0x5000u, runtime);
    Check(runtime.events.size() == 2u,
          "negative signed list length is skipped");
}

void OneTriangle() {
    Runtime runtime;
    Renderer::Render(0x5000u, runtime);
    Check(runtime.events[0] == Event('R', 0x5004u) &&
              runtime.events[1] == Event('R', 0x5008u) &&
              runtime.events[2] == Event('A', 216u),
          "six 36-byte vertices allocated for one triangle");
    Check(std::count(runtime.events.begin(), runtime.events.end(),
                     Event('R', 0x5004u)) == 10 &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         Event('R', 0x6000u)) == 9 &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         Event('G', 0u)) == 3 &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         Event('G', 1u)) == 3 &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         Event('G', 2u)) == 3,
          "three components each reread list and call vertex getter");
    constexpr std::uint32_t kColor = 0xdc1414f0u;
    const std::uint32_t expected[6][3] = {
        {1u, 2u, 3u}, {4u, 5u, 6u}, {4u, 5u, 6u},
        {7u, 8u, 9u}, {7u, 8u, 9u}, {1u, 2u, 3u}};
    for (std::uint32_t vertex = 0; vertex < 6u; ++vertex) {
        const auto base = 0xa000u + vertex * 36u;
        for (std::uint32_t component = 0; component < 3u; ++component)
            Check(runtime.rendered.at(base + component * 4u) ==
                      expected[vertex][component],
                  "A-B, B-C and C-A segment positions");
        Check(runtime.rendered.at(base + 24u) == kColor,
              "each vertex receives fixed debug line color");
    }
    Check(runtime.writes.size() == 24u &&
              runtime.writes[12u].first == 0xa000u + 3u * 36u + 8u &&
              runtime.writes[16u].first == 0xa000u + 4u * 36u + 8u,
          "C vertex z words are written before x and y as in IDA");
    Check(runtime.stateChanges ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {7u, 1u}, {1u, 0u}, {6u, 1u}, {7u, 2u}}),
          "render states are applied and restored in IDA order");
    Check(std::vector<std::string>(runtime.events.end() - 8,
                                   runtime.events.end()) ==
              (std::vector<std::string>{
                  Event('S', 7u), Event('S', 1u), Event('T', 6u),
                  Event('P', 1u), "E", Event('S', 6u),
                  Event('S', 7u), Event('F', 0xa000u)}),
          "draw, state restoration and free order");
}

void FailureAndMultiple() {
    Runtime runtime;
    runtime.transformResult = 0u;
    Renderer::Render(0x5000u, runtime);
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    Event('P', 1u)) == runtime.events.end() &&
              std::find(runtime.events.begin(), runtime.events.end(), "E") ==
                  runtime.events.end() &&
              runtime.events.back() == Event('F', 0xa000u),
          "failed transform skips primitive and end, still restores and frees");

    runtime = Runtime{};
    runtime.buffer = 0u;
    Renderer::Render(0x5000u, runtime);
    Check(runtime.events.back() == Event('S', 7u) &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        Event('T', 6u)) != runtime.events.end(),
          "null allocation still follows transform path but skips free");

    runtime = Runtime{};
    runtime.words[0x5008u] = 0x6008u;
    runtime.words[0x6004u] = 0x9010u;
    Renderer::Render(0x5000u, runtime);
    Check(runtime.events[2] == Event('A', 432u) &&
              runtime.writes.size() == 48u &&
              runtime.rendered.at(0xa000u + 6u * 36u) == 11u &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         Event('R', 0x6004u)) == 9,
          "two triangles use separate list entries and 12 vertices");

    pal4::GameArenaCollisionDebugRuntimeStub stub;
    Renderer::Render(0u, stub);
}

}  // namespace

int main() {
    try {
        NoTriangles();
        OneTriangle();
        FailureAndMultiple();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
