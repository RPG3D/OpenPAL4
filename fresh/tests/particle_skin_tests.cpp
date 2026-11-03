#include "pal4/particle_visual_object.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(float actual, float expected) {
    return std::abs(actual - expected) < 0.0001f;
}

struct Runtime final : pal4::IParticleSkinRuntime {
    int morph_targets = 1;
    std::uint32_t skin{};
    std::vector<pal4::ParticleVisualPoint> vertices;
    std::vector<std::array<std::uint8_t, 4u>> indices;
    std::vector<std::array<float, 4u>> weights;
    std::unordered_map<std::uint32_t, pal4::ParticleVisualPoint> buffer;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint8_t> bone_order;
    int frame_transforms{};
    std::uint32_t next_allocation = 0x1000u;

    std::uint32_t SourceGeometry(std::uint32_t source) override {
        Check(source == 0x5000u, "geometry comes from model +56");
        calls.emplace_back("geometry");
        return 0x6000u;
    }
    std::uint32_t Skin(std::uint32_t geometry) override {
        Check(geometry == 0x6000u, "skin query targets source geometry");
        calls.emplace_back("skin");
        return skin;
    }
    int MorphTargetCount(std::uint32_t geometry) override {
        Check(geometry == 0x6000u, "morph-target count targets source geometry");
        calls.emplace_back("morph_targets");
        return morph_targets;
    }
    std::uint32_t VertexCount(std::uint32_t geometry) override {
        Check(geometry == 0x6000u, "vertex count targets source geometry");
        calls.emplace_back("vertex_count");
        return static_cast<std::uint32_t>(vertices.size());
    }
    std::uint32_t Hierarchy(std::uint32_t source) override {
        Check(source == 0x5000u, "bone hierarchy comes from model source");
        calls.emplace_back("hierarchy");
        return 0x7000u;
    }
    pal4::ParticleVisualPoint GeometryVertex(
        std::uint32_t geometry, std::uint32_t index) override {
        Check(geometry == 0x6000u && index < vertices.size(),
              "vertex is read by native index");
        calls.emplace_back("vertex");
        return vertices[index];
    }
    std::array<std::uint8_t, 4u> BoneIndices(
        std::uint32_t input_skin, std::uint32_t vertex) override {
        Check(input_skin == skin && vertex < indices.size(),
              "packed bone indices come from skin");
        return indices[vertex];
    }
    std::array<float, 4u> BoneWeights(
        std::uint32_t input_skin, std::uint32_t vertex) override {
        Check(input_skin == skin && vertex < weights.size(),
              "bone weights come from skin");
        return weights[vertex];
    }
    pal4::ParticleVisualPoint TransformBoneVertex(
        std::uint32_t input_skin, std::uint32_t hierarchy,
        std::uint8_t bone, pal4::ParticleVisualPoint vertex) override {
        Check(input_skin == skin && hierarchy == 0x7000u,
              "bone transform uses skin and hierarchy");
        bone_order.push_back(bone);
        return {vertex.x + static_cast<float>(bone),
                vertex.y + static_cast<float>(bone * 2u),
                vertex.z + static_cast<float>(bone * 3u)};
    }
    pal4::ParticleVisualPoint TransformFrameVertex(
        std::uint32_t frame, pal4::ParticleVisualPoint vertex) override {
        Check(frame == 0x8000u, "unskinned transform uses model +44");
        ++frame_transforms;
        return {vertex.x + 10.0f, vertex.y + 20.0f, vertex.z + 30.0f};
    }
    std::uint32_t AllocatePointBuffer(std::uint32_t bytes) override {
        allocations.push_back(bytes);
        calls.emplace_back("allocate");
        const auto address = next_allocation;
        next_allocation += 0x1000u;
        return address;
    }
    pal4::ParticleVisualPoint ReadPointBuffer(
        std::uint32_t address) override {
        calls.emplace_back("read_point");
        return buffer.at(address);
    }
    void WritePointBuffer(std::uint32_t address,
                          pal4::ParticleVisualPoint point) override {
        calls.emplace_back("write_point");
        buffer[address] = point;
    }
    void FreePointBuffer(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
};

void TestEarlyReturn() {
    Runtime runtime;
    runtime.morph_targets = 3;
    pal4::ParticleVisualState state;
    state.WriteWord(56u, 0x5000u);
    state.WriteWord(64u, 0x1000u);
    state.WriteWord(68u, 0x1024u);
    Check(pal4::ParticleVisualObject::UpdateSkinAndBones(
              state, runtime) == 3 &&
              runtime.calls == std::vector<std::string>{
                  "geometry", "skin", "morph_targets"} &&
              state.ReadWord(68u) == 0x1024u,
          "multiple morph targets return before clearing cached points");
    runtime.morph_targets = 1;
    runtime.calls.clear();
    Check(pal4::ParticleVisualObject::UpdateSkinAndBones(
              state, runtime) == 0 &&
              state.ReadWord(68u) == state.ReadWord(64u) &&
              runtime.calls == std::vector<std::string>{
                  "geometry", "skin", "morph_targets",
                  "vertex_count", "hierarchy"},
          "zero vertices clear the logical point count without allocation");
}

void TestUnskinnedAndBufferGrowth() {
    Runtime runtime;
    runtime.vertices = {{1.0f, 2.0f, 3.0f},
                        {4.0f, 5.0f, 6.0f},
                        {7.0f, 8.0f, 9.0f}};
    pal4::ParticleVisualState state;
    state.WriteWord(56u, 0x5000u);
    state.WriteWord(44u, 0x8000u);
    Check(pal4::ParticleVisualObject::UpdateSkinAndBones(
              state, runtime) == 3 &&
              runtime.frame_transforms == 3 &&
              runtime.allocations ==
                  std::vector<std::uint32_t>{12u, 24u, 48u} &&
              state.ReadWord(64u) == 0x3000u &&
              state.ReadWord(68u) == 0x3024u &&
              state.ReadWord(72u) == 0x3030u,
          "unskinned points transform and grow capacity 1, 2, 4");
    Check(Near(runtime.buffer.at(0x3000u).x, 11.0f) &&
              Near(runtime.buffer.at(0x300cu).y, 25.0f) &&
              Near(runtime.buffer.at(0x3018u).z, 39.0f) &&
              runtime.calls.front() == "geometry",
          "growth copies prior points in vertex order");

    runtime.allocations.clear();
    runtime.calls.clear();
    runtime.frame_transforms = 0;
    state.WriteWord(124u, 1u);
    state.WriteByte(53u, 1u);
    Check(pal4::ParticleVisualObject::UpdateSkinAndBones(
              state, runtime) == 3 &&
              runtime.allocations.empty() &&
              runtime.frame_transforms == 0 &&
              state.ReadWord(68u) == 0x3024u &&
              Near(runtime.buffer.at(0x3000u).x, 1.0f),
          "type one with byte 53 skips transform and reuses point capacity");
    state.WriteByte(53u, 0u);
    state.WriteByte(4u, 1u);
    runtime.frame_transforms = 0;
    (void)pal4::ParticleVisualObject::UpdateSkinAndBones(state, runtime);
    Check(runtime.frame_transforms == 0,
          "byte four independently suppresses frame transformation");
}

void TestSkinnedWeights() {
    Runtime runtime;
    runtime.skin = 0x9000u;
    runtime.vertices = {{1.0f, 2.0f, 3.0f},
                        {1.0f, 2.0f, 3.0f},
                        {1.0f, 2.0f, 3.0f},
                        {1.0f, 2.0f, 3.0f}};
    runtime.indices = {{{2u, 0u, 0u, 0u},
                        {1u, 2u, 0u, 0u},
                        {1u, 2u, 3u, 0u},
                        {1u, 0u, 0u, 4u}}};
    runtime.weights = {{{1.0f, 0.0f, 0.0f, 0.0f},
                        {0.25f, 0.75f, 0.0f, 0.0f},
                        {0.2f, 0.3f, 0.5f, 0.0f},
                        {0.1f, 0.2f, 0.3f, 0.4f}}};
    pal4::ParticleVisualState state;
    state.WriteWord(56u, 0x5000u);
    Check(pal4::ParticleVisualObject::UpdateSkinAndBones(
              state, runtime) == 4 &&
              runtime.bone_order == std::vector<std::uint8_t>{
                  2u, 2u, 1u, 3u, 2u, 1u, 4u, 0u, 0u, 1u} &&
              runtime.frame_transforms == 0,
          "skinned path descends from highest present slot to slot zero");
    const auto begin = state.ReadWord(64u);
    Check(Near(runtime.buffer.at(begin).x, 3.0f) &&
              Near(runtime.buffer.at(begin + 12u).x, 2.75f) &&
              Near(runtime.buffer.at(begin + 24u).x, 3.3f) &&
              Near(runtime.buffer.at(begin + 36u).x, 2.7f) &&
              Near(runtime.buffer.at(begin + 36u).y, 5.4f) &&
              Near(runtime.buffer.at(begin + 36u).z, 8.1f),
          "up to four weighted bone positions accumulate in each point");
}

}  // namespace

int main() {
    TestEarlyReturn();
    TestUnskinnedAndBufferGrowth();
    TestSkinnedWeights();
}
