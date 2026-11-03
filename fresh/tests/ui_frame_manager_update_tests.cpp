#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr double kElapsed = 1.234567890123;

struct Runtime final : pal4::IUiFrameManagerUpdateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, bool> visible;
    std::vector<std::string> events;
    std::vector<std::uint32_t> updated_frames;
    std::uint32_t hint_updated = 0;
    std::uint32_t pulse_system = 0;
    std::uint32_t rotated_collection = 0;
    std::uint32_t timed_effects = 0xffffffffu;
    std::uint32_t animation_list = 0;
    std::uint32_t animation_return = 0x1234u;
    std::uint32_t list_begin_reads = 0;
    std::uint32_t list_end_reads = 0;
    bool change_special_fields = false;
    bool append_on_first_frame = false;
    bool throw_pulse = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x518cu) ++list_begin_reads;
        if (address == 0x5190u) ++list_end_reads;
        events.emplace_back("read " + std::to_string(address));
        return words[address];
    }
    bool IsVisible(std::uint32_t frame) override {
        events.emplace_back("visible " + std::to_string(frame));
        if (change_special_fields && frame == 0xa100u)
            words[0x51b8u] = 0xa200u;
        if (change_special_fields && frame == 0xb100u)
            words[0x51d0u] = 0xb200u;
        return visible[frame];
    }
    void InjectTimePulse(std::uint32_t system,
                         float elapsed) override {
        events.emplace_back("pulse");
        pulse_system = system;
        Check(elapsed == static_cast<float>(kElapsed),
              "time pulse receives narrowed float");
        if (throw_pulse) throw std::runtime_error("pulse failure");
    }
    void RotateGameObjects(std::uint32_t collection,
                           double elapsed) override {
        events.emplace_back("rotate");
        rotated_collection = collection;
        Check(elapsed == kElapsed,
              "object collection receives original double");
    }
    void UpdateJumpHint(std::uint32_t frame,
                        float elapsed) override {
        events.emplace_back("hint");
        hint_updated = frame;
        Check(elapsed == static_cast<float>(kElapsed),
              "jump hint receives narrowed float");
    }
    void UpdateFrame(std::uint32_t frame,
                     double elapsed) override {
        events.emplace_back("frame " + std::to_string(frame));
        updated_frames.push_back(frame);
        Check(elapsed == kElapsed,
              "dialog vtable receives original double");
        if (append_on_first_frame && frame == 0xc100u) {
            words[0x6008u] = 0xc300u;
            words[0x5190u] = 0x600cu;
        }
    }
    void UpdateTimedEffects(std::uint32_t effects,
                            float elapsed) override {
        events.emplace_back("effects");
        timed_effects = effects;
        Check(elapsed == static_cast<float>(kElapsed),
              "timed effects receive narrowed float");
    }
    std::uint32_t UpdateAnimations(std::uint32_t list,
                                   float elapsed) override {
        events.emplace_back("animations");
        animation_list = list;
        Check(elapsed == static_cast<float>(kElapsed),
              "animations receive same narrowed float");
        return animation_return;
    }
};

void CheckAbsentSubsystems() {
    Runtime runtime;
    Check(pal4::UiFrameManagerSingleton::Update(
              0x5000u, kElapsed, runtime) == 0x1234u,
          "final animation result returned");
    Check(runtime.pulse_system == 0u &&
              runtime.rotated_collection == 0u &&
              runtime.hint_updated == 0u &&
              runtime.updated_frames.empty(),
          "absent subsystems are skipped");
    Check(runtime.timed_effects == 0u &&
              runtime.animation_list == 0x51dcu,
          "effect and animation stages run even without UI system");
    Check(runtime.list_begin_reads == 1u &&
              runtime.list_end_reads == 0u &&
              runtime.events.back() == "animations",
          "null list exits without reading end");
}

void CheckVisibleAndDynamicFrames() {
    Runtime runtime;
    runtime.words[0x5198u] = 0x8800u;
    runtime.words[0x519cu] = 0x8900u;
    runtime.words[0x51b8u] = 0xa100u;
    runtime.words[0x51d0u] = 0xb100u;
    runtime.words[0x518cu] = 0x6000u;
    runtime.words[0x5190u] = 0x6008u;
    runtime.words[0x6000u] = 0xc100u;
    runtime.words[0x6004u] = 0xc200u;
    runtime.words[0x51a0u] = 0xd000u;
    runtime.visible[0xa100u] = true;
    runtime.visible[0xb100u] = true;
    runtime.visible[0xc100u] = true;
    runtime.visible[0xc200u] = false;
    runtime.visible[0xc300u] = true;
    runtime.change_special_fields = true;
    runtime.append_on_first_frame = true;

    Check(pal4::UiFrameManagerSingleton::Update(
              0x5000u, kElapsed, runtime) == 0x1234u,
          "full update returns animation result");
    Check(runtime.pulse_system == 0x8800u &&
              runtime.rotated_collection == 0x8900u &&
              runtime.hint_updated == 0xa200u,
          "system, object collection and re-read hint pointer");
    Check(runtime.updated_frames ==
              std::vector<std::uint32_t>{0xb200u, 0xc100u, 0xc300u},
          "visible special frame and dynamically appended frame update");
    Check(runtime.list_begin_reads == 4u &&
              runtime.list_end_reads == 4u,
          "frame collection bounds re-read on every iteration");
    Check(runtime.timed_effects == 0xd000u &&
              runtime.animation_list == 0x51dcu,
          "effect object and animation list address");
    Check(runtime.events.back() == "animations",
          "animations are final update stage");
}

void CheckInvisibleAndExceptionPaths() {
    Runtime hidden;
    hidden.words[0x51b8u] = 0xa100u;
    hidden.words[0x51d0u] = 0xb100u;
    Check(pal4::UiFrameManagerSingleton::Update(
              0x5000u, kElapsed, hidden) == 0x1234u &&
              hidden.hint_updated == 0u &&
              hidden.updated_frames.empty(),
          "invisible special windows are not updated");

    Runtime throwing;
    throwing.words[0x5198u] = 0x8800u;
    throwing.throw_pulse = true;
    bool threw = false;
    try {
        (void)pal4::UiFrameManagerSingleton::Update(
            0x5000u, kElapsed, throwing);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) == "pulse failure";
    }
    Check(threw && throwing.events.back() == "pulse" &&
              throwing.animation_list == 0u,
          "earlier update exception stops later stages");
}

}  // namespace

int main() {
    try {
        CheckAbsentSubsystems();
        CheckVisibleAndDynamicFrames();
        CheckInvisibleAndExceptionPaths();
        std::cout << "ui_frame_manager_update_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
