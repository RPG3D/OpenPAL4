#include "pal4/base_object_component.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace {

using Event = std::pair<char, std::uint32_t>;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IBaseObjectComponentRuntime {
    std::vector<Event> events;
    std::uint32_t world{0x3000u};
    bool first_world_query{};

    std::uint32_t ReadDword(std::uint32_t address) override {
        Check(address == 0x2004u,
              "rotation reads clump frame at +4");
        events.emplace_back('D', address);
        return 0x4000u;
    }
    void RotateFrame(std::uint32_t frame, std::uint32_t axis,
                     float angle, std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 1u &&
                  ((axis == 0x8a76b4u && angle == 1.0f) ||
                   (axis == 0x8a76c0u && angle == -2.0f) ||
                   (axis == 0x8a76ccu && angle == 3.0f)),
              "frame rotates around IDA X/Y/Z axes with scaled angles");
        events.emplace_back('R', axis);
    }
    void UpdateFrame(std::uint32_t frame) override {
        Check(frame == 0x4000u,
              "frame update follows all rotations");
        events.emplace_back('U', frame);
    }
    std::uint32_t AddClumpToWorld(std::uint32_t selected_world,
                                  std::uint32_t clump) override {
        Check(selected_world == 0x3000u && clump == 0x2000u,
              "add uses supplied world and object clump");
        events.emplace_back('A', clump);
        return 0x9999u;
    }
    std::uint32_t WorldFromClump(std::uint32_t clump) override {
        Check(clump == 0x2000u,
              "world lookup uses object clump");
        events.emplace_back('W', clump);
        first_world_query = true;
        return world;
    }
    void RemoveClumpFromWorld(std::uint32_t selected_world,
                              std::uint32_t clump) override {
        Check(first_world_query && selected_world == 0x3000u &&
                  clump == 0x2000u,
              "remove uses the second world lookup result");
        events.emplace_back('M', clump);
    }
    std::uint32_t UpdateClumpTransform(
        pal4::BaseObjectState&,
        const pal4::BaseObjectVector3& position,
        std::uint32_t mode) override {
        Check(position.x == 7.0f && position.y == 8.0f &&
                  position.z == 9.0f && mode == 0u,
              "position passes vector and zero mode to resource manager");
        events.emplace_back('P', 0u);
        return 0x8888u;
    }
    void UpdateClumpPosition(
        pal4::BaseObjectState&,
        const pal4::BaseObjectVector3& scale,
        std::uint32_t mode) override {
        Check(scale.x == 7.0f && scale.y == 8.0f &&
                  scale.z == 9.0f && mode == 0u,
              "scale passes vector and zero mode to resource manager");
        events.emplace_back('S', 0u);
    }
};

void CheckRotation() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    state.Write(0x174u, 0xffffff00u);
    pal4::BaseObjectComponent::Rotate(state, 0.5, runtime);
    Check(runtime.events.empty(), "disabled rotation skips clump access");
    state.Write(0x174u, 0xffffff01u);
    pal4::BaseObjectComponent::Rotate(state, 0.5, runtime);
    Check(runtime.events.empty(), "missing clump skips frame rotation");
    state.Write(0x14u, 0x2000u);
    state.Write(0x178u, std::bit_cast<std::uint32_t>(2.0f));
    state.Write(0x17cu, std::bit_cast<std::uint32_t>(-4.0f));
    state.Write(0x180u, std::bit_cast<std::uint32_t>(6.0f));
    pal4::BaseObjectComponent::Rotate(state, 0.5, runtime);
    Check(runtime.events == std::vector<Event>{
              {'D', 0x2004u}, {'R', 0x8a76b4u},
              {'R', 0x8a76c0u}, {'R', 0x8a76ccu},
              {'U', 0x4000u}},
          "rotation applies X/Y/Z then updates objects");
}

void CheckWorldAndTransform() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    const pal4::BaseObjectVector3 vector{7.0f, 8.0f, 9.0f};
    Check(pal4::BaseObjectComponent::AddToWorld(state, 0x3000u,
                                                 runtime) == 0u &&
              pal4::BaseObjectComponent::SetPosition(state, vector,
                                                      runtime) == 0u &&
              runtime.events.empty(),
          "missing clump skips world and transform helpers");
    pal4::BaseObjectComponent::RemoveFromWorld(state, runtime);
    pal4::BaseObjectComponent::SetScale(state, vector, runtime);
    Check(runtime.events.empty(),
          "missing clump skips world removal and scale");

    state.Write(0x14u, 0x2000u);
    Check(pal4::BaseObjectComponent::AddToWorld(state, 0u,
                                                 runtime) == 0x2000u &&
              runtime.events.empty(),
          "null world returns clump without adding it");
    Check(pal4::BaseObjectComponent::AddToWorld(state, 0x3000u,
                                                 runtime) == 0x9999u &&
              pal4::BaseObjectComponent::SetPosition(state, vector,
                                                      runtime) == 0x8888u,
          "world add and position forward service results");
    pal4::BaseObjectComponent::SetScale(state, vector, runtime);
    runtime.world = 0u;
    pal4::BaseObjectComponent::RemoveFromWorld(state, runtime);
    Check(runtime.events == std::vector<Event>{
              {'A', 0x2000u}, {'P', 0u}, {'S', 0u},
              {'W', 0x2000u}},
          "null parent world prevents second lookup and removal");
    runtime.events.clear();
    runtime.world = 0x3000u;
    runtime.RemoveFromWorld(state);
    Check(runtime.events == std::vector<Event>{
              {'W', 0x2000u}, {'W', 0x2000u}, {'M', 0x2000u}},
          "default cleanup runtime delegates to two-lookup world removal");
}

void CheckRotationSetter() {
    pal4::BaseObjectState state{};
    state.Write(0x174u, 0x12345678u);
    const pal4::BaseObjectVector3 rotation{2.0f, -4.0f, 6.0f};
    Check(pal4::BaseObjectComponent::SetRotation(
              state, 0xabu, rotation) ==
              std::bit_cast<std::uint32_t>(-4.0f) &&
              state.Read(0x174u) == 0x123456abu &&
              state.Read(0x178u) == std::bit_cast<std::uint32_t>(2.0f) &&
              state.Read(0x17cu) == std::bit_cast<std::uint32_t>(-4.0f) &&
              state.Read(0x180u) == std::bit_cast<std::uint32_t>(6.0f),
          "rotation setter writes one flag byte and three raw float words");
}

}  // namespace

int main() {
    CheckRotation();
    CheckWorldAndTransform();
    CheckRotationSetter();
}
