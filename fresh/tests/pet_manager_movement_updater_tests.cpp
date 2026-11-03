#include "pal4/pet_manager_movement_updater.h"

#include <bit>
#include <cctype>
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

struct Runtime final : pal4::PetManagerMovementRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t mode{};
    double distance{12.0};
    pal4::PetVector3 clamped{-0.6f, 0.0f, 0.8f};
    pal4::PetVector3 facing{};
    pal4::PetVector3 position{};
    std::string current_name{"idle"};
    std::string selected_name;
    std::uint32_t select_result{0x9999u};
    std::uint32_t magnitude_calls{};
    std::uint32_t clamp_calls{};
    std::uint32_t facing_calls{};
    std::uint32_t position_calls{};
    bool change_source_after_magnitude{};
    float expected_raw_x{10.0f};

    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x5670u, "mode writes only pet speed cap");
        events.emplace_back("cap");
        words[address] = value;
    }
    std::uint32_t ActorControl() override {
        events.emplace_back("controller");
        return 0x7000u;
    }
    std::uint32_t ActorMode(std::uint32_t control) override {
        Check(control == 0x7000u, "mode uses actor control singleton");
        events.emplace_back("mode");
        return mode;
    }
    double Magnitude(const pal4::PetVector3& vector) override {
        Check(Near(vector.x, -10.0f) && Near(vector.y, 0.0f) &&
                  Near(vector.z, 0.0f),
              "distance input is pet minus source in the horizontal plane");
        ++magnitude_calls;
        events.emplace_back("magnitude");
        if (change_source_after_magnitude) words[0x5658u] = 0xa000u;
        return distance;
    }
    void ClampDirection(pal4::PetVector3& direction) override {
        Check(Near(direction.x, expected_raw_x) && Near(direction.y, 6.5f) &&
                  Near(direction.z, 0.0f),
              "approach direction includes source height and pet vertical offset");
        ++clamp_calls;
        events.emplace_back("clamp");
        direction = clamped;
    }
    double NormalizeFloatRange(float dot) override {
        Check(Near(dot, clamped.z),
              "angle helper receives dot with constant (0,0,1)");
        events.emplace_back("angle");
        return 1.0471975511965976;
    }
    void SetFacingTarget(std::uint32_t pet, const pal4::PetVector3& target,
                         std::uint32_t value) override {
        Check(pet == 0x5000u && value == 0u,
              "facing virtual arguments");
        facing = target;
        ++facing_calls;
        events.emplace_back("facing");
    }
    void SetPosition(std::uint32_t pet,
                     const pal4::PetVector3& target) override {
        Check(pet == 0x5000u, "position virtual receives pet");
        position = target;
        ++position_calls;
        events.emplace_back("position");
    }
    std::string_view AnimationName(std::uint32_t animation) override {
        Check(animation == 0x5170u, "animation object offset");
        events.emplace_back("animation-name");
        return current_name;
    }
    int CompareNames(std::string_view current,
                     std::string_view expected) override {
        events.emplace_back("compare");
        if (current.size() != expected.size()) return 1;
        for (std::size_t i = 0; i < current.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(current[i])) !=
                std::tolower(static_cast<unsigned char>(expected[i])))
                return 1;
        }
        return 0;
    }
    std::uint32_t SelectAnimation(std::uint32_t animation,
                                  std::string_view name) override {
        Check(animation == 0x5170u, "select animation object offset");
        selected_name = name;
        events.emplace_back("select");
        return select_result;
    }
};

void Setup(Runtime& runtime) {
    runtime.words[0x5658u] = 0x9000u;
    runtime.words[0x91f0u] = 1u;
    runtime.words[0x50acu] = std::bit_cast<std::uint32_t>(1.0f);
    runtime.words[0x50b0u] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[0x50b4u] = std::bit_cast<std::uint32_t>(3.0f);
    runtime.words[0x90acu] = std::bit_cast<std::uint32_t>(11.0f);
    runtime.words[0x90b0u] = std::bit_cast<std::uint32_t>(7.0f);
    runtime.words[0x90b4u] = std::bit_cast<std::uint32_t>(3.0f);
    runtime.words[0x90ecu] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[0x5664u] = std::bit_cast<std::uint32_t>(1.5f);
    runtime.words[0x5668u] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.words[0x5670u] = std::bit_cast<std::uint32_t>(5.0f);
}

void InactiveAndNearPaths() {
    Runtime runtime;
    pal4::PetMovementScratch scratch{};
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0u &&
              runtime.events.size() == 1u,
          "missing source returns zero before actor control access");
    Setup(runtime);
    runtime.words[0x91f0u] = 0u;
    runtime.events.clear();
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0x9000u &&
              runtime.events.size() == 2u,
          "inactive source returns the source register without movement");

    runtime.words[0x91f0u] = 1u;
    runtime.distance = 2.0;
    runtime.current_name = "c01";
    runtime.events.clear();
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0u &&
              runtime.selected_name.empty() && runtime.clamp_calls == 0u &&
              Near(scratch.direction.x, -10.0f),
          "threshold equality selects idle path without restarting C01");
    runtime.current_name = "other";
    runtime.events.clear();
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0x9999u &&
              runtime.selected_name == "C01" &&
              runtime.events[runtime.events.size() - 2u] == "compare" &&
              runtime.events.back() == "select",
          "near path selects C01 when the case-insensitive comparison differs");
}

void MovementAndAnimation() {
    Runtime runtime;
    Setup(runtime);
    pal4::PetMovementScratch scratch{};
    Check(pal4::PetManagerMovementUpdater::kDirectionAddress == 0x8e4334u &&
              pal4::PetManagerMovementUpdater::kNextPositionAddress ==
                  0x8e4328u,
          "scratch vector addresses match IDA globals");
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0x9999u &&
              runtime.words[0x5670u] == 0x42a00000u &&
              runtime.selected_name == "C02",
          "mode zero writes 80 speed cap and selects moving animation");
    Check(runtime.magnitude_calls == 1u && runtime.clamp_calls == 1u &&
              runtime.facing_calls == 1u && runtime.position_calls == 1u &&
              Near(scratch.direction.x, -0.6f) &&
              Near(scratch.next_position.x, -2.0f) &&
              Near(scratch.next_position.y, 2.0f) &&
              Near(scratch.next_position.z, 7.0f),
          "speed times elapsed scales the clamped direction from pet position");
    Check(Near(runtime.facing.x, 0.0f) &&
              Near(runtime.facing.y, 300.0f) &&
              Near(runtime.facing.z, 0.0f) &&
              runtime.events[runtime.events.size() - 5u] == "facing" &&
              runtime.events[runtime.events.size() - 4u] == "position",
          "negative X converts angle to 300 degrees before position update");

    runtime.current_name = "c02";
    runtime.events.clear();
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0u &&
              runtime.events.back() == "compare",
          "already selected C02 returns the zero comparison result");
}

void SpeedLimitsAndModes() {
    Runtime runtime;
    Setup(runtime);
    pal4::PetMovementScratch scratch{};
    runtime.mode = 2u;
    runtime.distance = 102.0;
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0x9999u &&
              runtime.words[0x5670u] == 0x43790000u &&
              Near(scratch.next_position.x, -29.0f),
          "mode two sets 249 cap and allows speed 200");
    runtime.mode = 1u;
    runtime.events.clear();
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0x9999u &&
              runtime.words[0x5670u] == 0x43290000u &&
              Near(scratch.next_position.x, -24.35f),
          "mode one caps candidate speed 200 at 169");
    runtime.mode = 0u;
    runtime.events.clear();
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0x9999u &&
              Near(scratch.next_position.x, -11.0f),
          "mode zero caps candidate speed 200 at 80");
    runtime.distance = 2.25;
    runtime.events.clear();
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0x9999u &&
              Near(scratch.next_position.x, 0.85f) &&
              Near(scratch.next_position.z, 3.2f),
          "movement speed has a one-unit floor after maximum check");
    runtime.mode = 3u;
    runtime.distance = 12.0;
    runtime.words[0x5670u] = std::bit_cast<std::uint32_t>(5.0f);
    runtime.events.clear();
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0x9999u &&
              runtime.words[0x5670u] ==
                  std::bit_cast<std::uint32_t>(5.0f) &&
              Near(scratch.next_position.x, 0.25f),
          "unhandled mode preserves prior speed cap");
}

void SourceIsReadAgainAfterMagnitude() {
    Runtime runtime;
    Setup(runtime);
    runtime.words[0xa0acu] = std::bit_cast<std::uint32_t>(21.0f);
    runtime.words[0xa0b0u] = std::bit_cast<std::uint32_t>(7.0f);
    runtime.words[0xa0b4u] = std::bit_cast<std::uint32_t>(3.0f);
    runtime.words[0xa0ecu] = std::bit_cast<std::uint32_t>(2.0f);
    runtime.change_source_after_magnitude = true;
    runtime.expected_raw_x = 20.0f;
    pal4::PetMovementScratch scratch{};
    Check(pal4::PetManagerMovementUpdater::Update(
              0x5000u, 0.25f, scratch, runtime) == 0x9999u &&
              runtime.clamp_calls == 1u,
          "far path rereads the source pointer after magnitude callback");
}

}  // namespace

int main() {
    InactiveAndNearPaths();
    MovementAndAnimation();
    SpeedLimitsAndModes();
    SourceIsReadAgainAfterMagnitude();
    std::cout << "4 pet movement test groups passed\n";
}
