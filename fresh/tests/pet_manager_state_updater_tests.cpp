#include "pal4/pet_manager_state_updater.h"

#include <bit>
#include <cmath>
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
    return std::abs(actual - expected) < 0.001f;
}

struct Runtime final : pal4::PetManagerUpdateRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    pal4::PetVector3 world{10.0f, 20.0f, 30.0f};
    pal4::PetVectorView direction{0x8800u, {-0.5f, 7.0f, 0.5f}};
    pal4::PetVector3 last_facing{};
    pal4::PetVector3 last_position{};
    float last_dot{};
    std::uint32_t vector_reads{};
    std::uint32_t d3d_reads{};
    std::uint32_t zero_address{};
    std::uint32_t movement_result{0x1111u};
    std::uint32_t position_result{0x2222u};
    std::uint32_t zero_result{0x3333u};
    double normalized_angle{1.0471975511965976};

    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void UpdateAndRenderClump(float elapsed) override {
        Check(elapsed == 0.25f, "resource update forwards elapsed time");
        events.emplace_back("clump");
    }
    void UpdateInterpolator(std::uint32_t animation, float elapsed) override {
        Check(animation == 0x5170u && elapsed == 0.25f,
              "animation update uses +0x170 and elapsed time");
        events.emplace_back("interpolator");
    }
    std::uint32_t UpdateMovement(std::uint32_t pet, float elapsed) override {
        Check(pet == 0x5000u && elapsed == 0.25f,
              "ordinary movement forwards object and time");
        events.emplace_back("movement");
        return movement_result;
    }
    void UpdateEmbeddedMotion(std::uint32_t embedded,
                              std::uint32_t argument,
                              float elapsed, float weight) override {
        Check(embedded == 0x5564u && argument == 0x99u &&
                  elapsed == 0.25f && weight == 1.0f,
              "embedded virtual call operands");
        events.emplace_back("embedded");
    }
    float ReadD3DVectorComponent(std::uint32_t embedded,
                                  std::uint32_t index) override {
        Check(embedded == 0x5564u && index < 3u,
              "D3D vector object and component index");
        ++d3d_reads;
        events.emplace_back("d3d");
        return index == 0u ? world.x : index == 1u ? world.y : world.z;
    }
    pal4::PetVectorView ReadCsbVector(std::uint32_t embedded) override {
        Check(embedded == 0x5564u, "CSB vector object offset");
        ++vector_reads;
        events.emplace_back("csb");
        return direction;
    }
    void ClampHorizontalDirection(pal4::PetVector3& value) override {
        Check(value.y == 0.0f, "vertical direction is removed before clamp");
        events.emplace_back("clamp");
    }
    double NormalizeFloatRange(float dot) override {
        last_dot = dot;
        events.emplace_back("angle");
        return normalized_angle;
    }
    void SetFacingTarget(std::uint32_t pet, const pal4::PetVector3& value,
                         std::uint32_t mode) override {
        Check(pet == 0x5000u && mode == 0u,
              "facing virtual uses object and zero mode");
        last_facing = value;
        events.emplace_back("facing");
    }
    std::uint32_t SetPosition(std::uint32_t pet,
                              const pal4::PetVector3& value) override {
        Check(pet == 0x5000u, "position virtual uses object");
        last_position = value;
        events.emplace_back("position");
        return position_result;
    }
    std::uint32_t ZeroDirectionReturn(std::uint32_t address) override {
        zero_address = address;
        events.emplace_back("zero-direction");
        return zero_result;
    }
};

void Enable(Runtime& runtime) {
    runtime.words[0x5654u] = 1u;
    runtime.words[0x51f0u] = 1u;
    runtime.words[0x5660u] = 1u;
    runtime.words[0x55f4u] = 0u;
    runtime.words[0x50a0u] = std::bit_cast<std::uint32_t>(3.0f);
    runtime.words[0x50a4u] = std::bit_cast<std::uint32_t>(4.0f);
    runtime.words[0x50a8u] = std::bit_cast<std::uint32_t>(5.0f);
    runtime.words[0x5664u] = std::bit_cast<std::uint32_t>(1.5f);
}

void InactiveAndOrdinaryMovement() {
    Runtime runtime;
    Check(pal4::PetManagerStateUpdater::Update(
              0x5000u, 0.25f, 0x99u, runtime) == 0u &&
              runtime.events.size() == 1u,
          "missing +0x654 exits before other reads");
    runtime.words[0x5654u] = 1u;
    runtime.events.clear();
    Check(pal4::PetManagerStateUpdater::Update(
              0x5000u, 0.25f, 0x99u, runtime) == 0u &&
              runtime.events.size() == 2u,
          "missing +0x1f0 exits before resource update");
    Enable(runtime);
    runtime.words[0x5660u] = 0u;
    runtime.events.clear();
    Check(pal4::PetManagerStateUpdater::Update(
              0x5000u, 0.25f, 0x99u, runtime) == 0x1111u,
          "ordinary movement result forwarded");
    Check(runtime.events == std::vector<std::string>{
              "read:22100", "read:20976", "clump", "interpolator",
              "read:22112", "movement"},
          "ordinary path updates clump and animation before movement");
}

void HeldObjectAndZeroDirection() {
    Runtime runtime;
    Enable(runtime);
    runtime.words[0x55f4u] = 0x7777u;
    Check(pal4::PetManagerStateUpdater::Update(
              0x5000u, 0.25f, 0x99u, runtime) == 0x7777u &&
              runtime.events.back() == "read:22004" &&
              runtime.d3d_reads == 0u,
          "held object exits after embedded virtual update");
    runtime.words[0x55f4u] = 0u;
    runtime.direction.value = {0.0f, 9.0f, 0.0f};
    runtime.events.clear();
    Check(pal4::PetManagerStateUpdater::Update(
              0x5000u, 0.25f, 0x99u, runtime) == 0x3333u &&
              runtime.zero_address == 0x8800u &&
              runtime.d3d_reads == 3u && runtime.vector_reads == 2u,
          "zero horizontal direction returns the injected register result");
    Check(runtime.events.back() == "zero-direction",
          "zero direction skips orientation and position virtuals");
}

void FacingAndPosition() {
    Runtime runtime;
    Enable(runtime);
    Check(pal4::PetManagerStateUpdater::Update(
              0x5000u, 0.25f, 0x99u, runtime) == 0x2222u,
          "active position virtual result forwarded");
    Check(runtime.vector_reads == 4u && runtime.d3d_reads == 3u &&
              Near(runtime.last_dot, 0.5f),
          "nonzero X direction reads each vector component and uses Z dot");
    Check(Near(runtime.last_facing.x, 3.0f) &&
              Near(runtime.last_facing.y, 300.0f) &&
              Near(runtime.last_facing.z, 5.0f),
          "negative X reflects angle over a full turn");
    Check(Near(runtime.last_position.x, 10.0f) &&
              Near(runtime.last_position.y, 21.5f) &&
              Near(runtime.last_position.z, 30.0f) &&
              runtime.events[runtime.events.size() - 2u] == "read:22116" &&
              runtime.events.back() == "position",
          "position adds +0x664 height after facing virtual");

    runtime.words[0x5604u] = 1u;
    runtime.vector_reads = 0u;
    runtime.events.clear();
    Check(pal4::PetManagerStateUpdater::Update(
              0x5000u, 0.25f, 0x99u, runtime) == 0x2222u &&
              Near(runtime.last_dot, -0.5f) &&
              Near(runtime.last_facing.y, 60.0f),
          "reverse flag negates the source vector before angle choice");
    Check(runtime.vector_reads == 4u,
          "reverse path preserves repeated CSB vector accesses");

    runtime.direction.value = {0.0f, 4.0f, 1.0f};
    runtime.vector_reads = 0u;
    Check(pal4::PetManagerStateUpdater::Update(
              0x5000u, 0.25f, 0x99u, runtime) == 0x2222u &&
              runtime.vector_reads == 5u,
          "zero X with nonzero Z enters the full path after second gate read");
}

}  // namespace

int main() {
    InactiveAndOrdinaryMovement();
    HeldObjectAndZeroDirection();
    FacingAndPosition();
    std::cout << "3 pet state update test groups passed\n";
}
