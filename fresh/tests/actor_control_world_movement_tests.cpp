#include "pal4/actor_control_world_movement.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kActor = 0x1000u;
constexpr std::uint32_t kDelta = 0x8000u;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void CheckVector(const pal4::ActorControlVec3& actual,
                 float x, float y, float z, const char* message) {
    Check(std::fabs(actual.x - x) < 0.00002f &&
              std::fabs(actual.y - y) < 0.00002f &&
              std::fabs(actual.z - z) < 0.00002f,
          message);
}

struct Runtime final : pal4::ActorControlDiscreteMovementRuntimeStub {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::uint32_t camera_result = 7u;
    std::uint32_t collision_result = 9u;
    std::uint32_t floor_result = 77u;
    std::uint32_t tag_reads = 0u;
    pal4::ActorControlVec3 written{};
    pal4::ActorControlVec3 minimap{};

    Runtime() {
        memory[kActor + 0x14u] = 0x2000u;
        memory[kActor + 0xacu] = std::bit_cast<std::uint32_t>(10.0f);
        memory[kActor + 0xb0u] = std::bit_cast<std::uint32_t>(20.0f);
        memory[kActor + 0xb4u] = std::bit_cast<std::uint32_t>(30.0f);
        memory[kDelta] = std::bit_cast<std::uint32_t>(1.0f);
        memory[kDelta + 4u] = std::bit_cast<std::uint32_t>(2.0f);
        memory[kDelta + 8u] = std::bit_cast<std::uint32_t>(3.0f);
    }
    std::uint32_t CameraSystem() override {
        calls.emplace_back("camera_system");
        return 0x6000u;
    }
    std::uint32_t CameraActiveResult(std::uint32_t system) override {
        Check(system == 0x6000u, "camera gate uses returned system");
        calls.emplace_back("camera_active");
        return camera_result;
    }
    void GetActiveCamera(std::uint32_t system) override {
        Check(system == 0x6000u, "active camera uses second system");
        calls.emplace_back("get_camera");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x5024u)
            return ++tag_reads == 1u ? 0xaau : 0xbbu;
        Check(memory.contains(address), "expected actor or input read");
        return memory[address];
    }
    double ActorProperty(std::uint32_t actor) override {
        Check(actor == kActor, "property receives actor");
        calls.emplace_back("property");
        return 4.0;
    }
    std::uint32_t Controller() override {
        calls.emplace_back("controller");
        return 0x5000u;
    }
    std::uint32_t MovementAdapter() override {
        calls.emplace_back("adapter");
        return 0x7000u;
    }
    void PreprocessMovement(std::uint32_t adapter,
                            pal4::ActorControlMovementGlobals& globals,
                            pal4::ActorControlVec3& delta,
                            std::uint32_t object, std::uint32_t first,
                            std::uint32_t second,
                            std::uint32_t third) override {
        Check(adapter == 0x7000u && object == 0x2000u &&
                  first == 1u && second == 1u && third == 1u &&
                  globals.upper_tag == 0xaau &&
                  globals.lower_tag == 0xbbu,
              "preprocess sees actor object, flags and global tags");
        calls.emplace_back("preprocess");
        CheckVector(delta, 1.0f, 2.0f, 3.0f,
                    "preprocess receives caller delta unchanged");
        delta.x += 0.25f;
    }
    void UpdateMovement(std::uint32_t adapter,
                        pal4::ActorControlVec3& position,
                        float lower, float upper,
                        std::uint32_t enabled) override {
        Check(adapter == 0x7000u &&
                  std::fabs(lower - 1.4f) < 0.00002f &&
                  std::fabs(upper - 2.12f) < 0.00002f &&
                  enabled == 1u,
              "update receives property radii and enable flag");
        calls.emplace_back("update");
        CheckVector(position, 11.25f, 22.0f, 33.0f,
                    "position includes preprocessed displacement");
        position.y += 1.0f;
    }
    std::uint32_t CanMoveResult(std::uint32_t adapter,
                                pal4::ActorControlVec3& position,
                                float lower, float upper) override {
        Check(adapter == 0x7000u &&
                  std::fabs(lower - 1.4f) < 0.00002f &&
                  std::fabs(upper - 2.12f) < 0.00002f,
              "collision receives property radii");
        calls.emplace_back("collision");
        CheckVector(position, 11.25f, 23.0f, 33.0f,
                    "collision sees update mutation");
        position.z += 0.5f;
        return collision_result;
    }
    void SetActorPosition(std::uint32_t actor,
                          const pal4::ActorControlVec3& position) override {
        Check(actor == kActor, "position write receives actor");
        calls.emplace_back("set_position");
        written = position;
    }
    void UpdateMinimap(const pal4::ActorControlVec3& position,
                       std::uint32_t grid) override {
        Check(grid == kActor + 0xa0u, "minimap receives actor grid");
        calls.emplace_back("minimap");
        minimap = position;
    }
    std::uint32_t UpdateFloorAttributesResult() override {
        calls.emplace_back("floor");
        return floor_result;
    }
};

}  // namespace

int main() {
    pal4::ActorControlMovementGlobals globals{};
    Runtime runtime;
    runtime.camera_result = 0u;
    Check(pal4::ActorControlWorldMovement::ApplyDelta(
              kActor, kDelta, globals, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "camera_system", "camera_active"},
          "inactive camera exits before reading actor or delta");

    runtime.calls.clear();
    runtime.camera_result = 7u;
    Check(pal4::ActorControlWorldMovement::ApplyDelta(
              0u, kDelta, globals, runtime) == 7u &&
              runtime.calls == std::vector<std::string>{
                  "camera_system", "camera_active"},
          "null actor returns raw nonzero camera result");

    runtime.calls.clear();
    runtime.collision_result = 0u;
    Check(pal4::ActorControlWorldMovement::ApplyDelta(
              kActor, kDelta, globals, runtime) == 0u,
          "rejected collision returns raw zero");
    Check(runtime.calls == std::vector<std::string>{
              "camera_system", "camera_active", "camera_system",
              "get_camera", "property", "property", "property",
              "controller", "controller", "adapter", "preprocess",
              "adapter", "update", "adapter", "collision"},
          "rejected path preserves service order and skips writes");
    CheckVector(globals.lower, 10.0f, 21.5f, 30.0f,
                "third global pair lower point");
    CheckVector(globals.upper, 10.0f, 22.5f, 30.0f,
                "third global pair upper point");

    Runtime accepted;
    Check(pal4::ActorControlWorldMovement::ApplyDelta(
              kActor, kDelta, globals, accepted) == 77u,
          "accepted path returns floor update result");
    CheckVector(accepted.written, 11.25f, 23.0f, 33.5f,
                "position write receives collision-mutated candidate");
    CheckVector(accepted.minimap, 11.25f, 23.0f, 33.5f,
                "minimap receives written position");
    Check(accepted.calls == std::vector<std::string>{
              "camera_system", "camera_active", "camera_system",
              "get_camera", "property", "property", "property",
              "controller", "controller", "adapter", "preprocess",
              "adapter", "update", "adapter", "collision",
              "set_position", "minimap", "floor"},
          "accepted path preserves write and floor order");

    pal4::ActorControlDiscreteMovementRuntimeStub stub;
    Check(pal4::ActorControlWorldMovement::ApplyDelta(
              kActor, kDelta, globals, stub) == 0u,
          "default runtime stub is linkable without game services");
}
