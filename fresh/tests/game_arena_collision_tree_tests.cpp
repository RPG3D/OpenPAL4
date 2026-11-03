#include "pal4/game_arena_collision_tree.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using Tree = pal4::GameArenaCollisionTree;
using Vector = pal4::CollisionVector;
using RawVector = pal4::CollisionRawVector;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string Event(char operation, std::uint32_t value) {
    return std::string(1, operation) + std::to_string(value);
}

struct Triangle {
    std::uint32_t tree;
    Vector vertices[3];
};

struct Runtime final : pal4::IGameArenaCollisionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x5108u, 0x8000u}, {0x8034u, 0u},
        {0x5008u, 0x9000u}, {0x500cu, 0x9010u},
        {0x7004u, 0x7500u}, {0x7018u, 0x7100u},
        {0x7110u, 2u}, {0x712cu, 0x7200u},
        {0x715cu, 0x7300u}, {0x7314u, 0x7400u}};
    std::map<std::uint32_t, std::uint16_t> halfWords{
        {0x7200u, 0u}, {0x7202u, 1u}, {0x7204u, 2u},
        {0x7208u, 0u}, {0x720au, 0u}, {0x720cu, 2u}};
    std::map<std::uint32_t, RawVector> vertices{
        {0x7400u, {0u, 0u, 0u}},
        {0x740cu, {1u, 0u, 0u}},
        {0x7418u, {0u, 1u, 0u}}};
    std::map<std::uint32_t, std::string> slotNames;
    std::vector<std::string> events;
    std::vector<Triangle> triangles;
    std::uint32_t allocation = 0xb000u;
    std::uint32_t visitedAtomic = 0x7000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back(Event('R', address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back(Event('W', address));
        words[address] = value;
    }
    std::string ReadCString(std::uint32_t address) override {
        events.push_back(Event('S', address));
        return address == 0x9000u ? "world" : "area";
    }
    bool TextEqualsIgnoreCase(std::uint32_t address,
                              std::string_view text) override {
        events.push_back(Event('C', address));
        return slotNames[address] == text;
    }
    void InitializeText(std::uint32_t, std::string_view) override {}
    void ReleaseObjectChildren(std::uint32_t) override {}
    void DestroyObject(std::uint32_t) override {}
    std::uint32_t FreeObject(std::uint32_t) override { return 0u; }

    std::uint32_t AllocateTree(std::uint32_t size) override {
        events.push_back(Event('A', size));
        return allocation;
    }
    std::uint32_t InitializeTree(std::uint32_t memory) override {
        events.push_back(Event('I', memory));
        return memory + 0x100u;
    }
    void ForAllAtomics(
        std::uint32_t clump, std::uint32_t arena,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                  pal4::IGameArenaCollisionRuntime&)) override {
        events.push_back(Event('F', clump));
        Check(callback(visitedAtomic, arena, *this) == visitedAtomic,
              "atomic visitor returns input atomic");
    }
    void SetupRenderStates(std::uint32_t tree) override {
        events.push_back(Event('P', tree));
    }
    void SetDebugRenderer(std::uint32_t tree) override {
        events.push_back(Event('D', tree));
    }
    std::uint16_t ReadHalfWord(std::uint32_t address) override {
        events.push_back(Event('H', address));
        return halfWords[address];
    }
    std::uint32_t GetObjectTransform(std::uint32_t frame) override {
        events.push_back(Event('G', frame));
        return frame + 1u;
    }
    RawVector TransformVertex(std::uint32_t vertex,
                              std::uint32_t transform) override {
        events.push_back(Event('T', vertex));
        Check(transform == 0x7501u, "frame transform forwarded");
        return vertices[vertex];
    }
    Vector ConvertCoordinates(const RawVector& raw) override {
        events.push_back(Event('V', raw[0] + raw[1] * 10u));
        return {static_cast<float>(raw[0]), static_cast<float>(raw[1]),
                static_cast<float>(raw[2])};
    }
    bool VectorsDiffer(const Vector& left, const Vector& right) override {
        events.push_back("N");
        return left != right;
    }
    void AddTriangle(std::uint32_t tree, const Vector& first,
                     const Vector& second, const Vector& third) override {
        events.push_back(Event('Q', tree));
        triangles.push_back({tree, {first, second, third}});
    }
};

void Builder() {
    Runtime runtime;
    Tree::BuildFromClump(0x5000u, 0u, runtime);
    Check(runtime.events.empty(), "null clump stops before cache lookup");

    runtime = Runtime{};
    runtime.words[0x5214u] = 1u;
    runtime.words[0x521cu] = 0xa000u;
    runtime.slotNames[0x51fcu] = "world";
    runtime.slotNames[0x5208u] = "area";
    Tree::BuildFromClump(0x5000u, 0x6000u, runtime);
    Check(runtime.words[0x5194u] == 0xa000u &&
              runtime.words[0x5218u] == 1u &&
              runtime.events.back() == Event('W', 0x5194u) &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        Event('A', 0x40u)) == runtime.events.end(),
          "cache hit marks slot and reuses tree without allocation");

    runtime = Runtime{};
    Tree::BuildFromClump(0x5000u, 0x6000u, runtime);
    Check(runtime.words[0x5194u] == 0xb100u &&
              runtime.triangles.size() == 1u &&
              runtime.triangles[0].tree == 0xb100u &&
              runtime.triangles[0].vertices[0] == Vector{0.f, 0.f, 0.f} &&
              runtime.triangles[0].vertices[1] == Vector{1.f, 0.f, 0.f} &&
              runtime.triangles[0].vertices[2] == Vector{0.f, 1.f, 0.f},
          "cache miss builds collision tree through atomic callback");
    const auto allocate = std::find(runtime.events.begin(), runtime.events.end(),
                                    Event('A', 0x40u));
    Check(allocate != runtime.events.end() &&
              std::vector<std::string>(allocate, allocate + 4) ==
                  (std::vector<std::string>{
                      Event('A', 0x40u), Event('I', 0xb000u),
                      Event('W', 0x5194u), Event('F', 0x6000u)}) &&
              runtime.events[runtime.events.size() - 4u] ==
                  Event('R', 0x5194u) &&
              runtime.events[runtime.events.size() - 3u] ==
                  Event('P', 0xb100u) &&
              runtime.events[runtime.events.size() - 2u] ==
                  Event('R', 0x5194u) &&
              runtime.events.back() == Event('D', 0xb100u),
          "allocation, visitor, render state and renderer follow IDA order");

    runtime = Runtime{};
    runtime.allocation = 0u;
    runtime.visitedAtomic = 0u;
    Tree::BuildFromClump(0x5000u, 0x6000u, runtime);
    Check(runtime.words[0x5194u] == 0u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        Event('I', 0u)) == runtime.events.end() &&
              runtime.events.back() == Event('D', 0u),
          "allocation failure still visits atomics and configures null tree");
}

void Triangles() {
    Runtime runtime;
    runtime.words[0x7018u] = 0u;
    Check(Tree::AddTriangles(0x7000u, 0x5000u, runtime) == 0x7000u &&
              runtime.events == std::vector<std::string>{Event('R', 0x7018u)},
          "geometry gate returns atomic");
    runtime = Runtime{};
    runtime.words[0x7110u] = 0u;
    Check(Tree::AddTriangles(0x7000u, 0x5000u, runtime) == 0x7000u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        Event('G', 0x7500u)) == runtime.events.end(),
          "nonpositive triangle count skips vertex processing");

    runtime = Runtime{};
    runtime.words[0x5194u] = 0xb100u;
    Check(Tree::AddTriangles(0x7000u, 0x5000u, runtime) == 0x7000u &&
              runtime.triangles.size() == 1u,
          "degenerate second triangle is discarded");
    const auto firstFrame = std::find(runtime.events.begin(), runtime.events.end(),
                                      Event('R', 0x7004u));
    Check(firstFrame != runtime.events.end() &&
              std::vector<std::string>(firstFrame, firstFrame + 15) ==
                  (std::vector<std::string>{
                      Event('R', 0x7004u), Event('G', 0x7500u),
                      Event('H', 0x7200u), Event('T', 0x7400u),
                      Event('R', 0x7004u), Event('G', 0x7500u),
                      Event('H', 0x7202u), Event('T', 0x740cu),
                      Event('R', 0x7004u), Event('G', 0x7500u),
                      Event('H', 0x7204u), Event('T', 0x7418u),
                      Event('V', 0u), Event('V', 1u), Event('V', 10u)}) &&
              std::count(runtime.events.begin(), runtime.events.end(), "N") == 4,
          "each triangle rereads frame and uses 8-byte indices; conversions follow transforms");

    pal4::GameArenaCollisionRuntimeStub stub;
    Tree::BuildFromClump(0u, 0u, stub);
    Check(Tree::AddTriangles(0u, 0u, stub) == 0u,
          "native collision runtime stub links");
}

}  // namespace

int main() {
    try {
        Builder();
        Triangles();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
