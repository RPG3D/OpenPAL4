#include "pal4/minimap_scene_ui_interaction.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMinimapSceneUiInteractionRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words{
        {0x8008u, 0x9000u}, {0xa1bcu, 4u}, {0xc004u, 0xd000u}};
    std::uint32_t minimap_object = 0x8000u;
    std::uint32_t game_object = 0xa000u;
    std::uint32_t trigger_object = 0xb000u;
    std::uint32_t text = 0u;
    pal4::MinimapUiPosition display_position{};
    std::vector<float> rounded;

    pal4::MinimapUiPosition AbsolutePosition(
        std::uint32_t window) override {
        Check(window == 0x7000u, "event window pointer");
        calls.emplace_back("absolute");
        return {10.0f, 20.0f};
    }
    pal4::MinimapUiPosition TransformCoordinates(
        pal4::MinimapUiPosition position) override {
        Check(position.x == 12.0f && position.y == 20.0f,
              "absolute x receives two-pixel offset");
        calls.emplace_back("transform");
        return {3.0f, 4.0f};
    }
    pal4::MinimapUiPosition GameStatePosition() override {
        calls.emplace_back("game-position");
        return {13.0f, 25.0f};
    }
    std::int32_t FloatToLong(float value) override {
        rounded.push_back(value);
        calls.emplace_back("ftol");
        return static_cast<std::int32_t>(value);
    }
    std::uint32_t FindMinimapObject(
        std::int32_t x, std::int32_t y, std::uint32_t mode) override {
        Check(x == 10 && y == 21 && mode == 1u,
              "minimap lookup offset and mode");
        calls.emplace_back("minimap-find");
        return minimap_object;
    }
    void CloseUtf8Window(std::uint32_t address) override {
        Check(address == 0x506cu, "UTF-8 hint member offset");
        calls.emplace_back("close");
    }
    pal4::MinimapUiPosition AbsoluteToRelative(
        pal4::MinimapUiPosition position) override {
        Check(position.x == 45.0f && position.y == 60.0f,
              "event display position applies minus-20 y offset");
        calls.emplace_back("relative");
        return {1.5f, 2.5f};
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words[address];
    }
    std::uint32_t FindGameObjectByName(
        std::uint32_t name) override {
        Check(name == 0x9000u, "minimap object name pointer");
        calls.emplace_back("game-find");
        return game_object;
    }
    std::uint32_t TriggerObject(std::int32_t index) override {
        Check(index == 4, "signed trigger index");
        calls.emplace_back("trigger-object");
        return trigger_object;
    }
    std::uint32_t TriggerSwitch(
        std::uint32_t object) override {
        Check(object == 0xb000u, "trigger object pointer");
        calls.emplace_back("trigger-switch");
        return 0xc000u;
    }
    std::uint32_t EmptyString() override {
        calls.emplace_back("empty-string");
        return 0xe000u;
    }
    void ConvertUtf8ToUnicode(
        std::uint32_t value, pal4::MinimapUiPosition position,
        std::uint32_t scale_bits, std::uint32_t flags,
        std::uint32_t enabled) override {
        Check(position.x == 1.5f && position.y == 2.5f &&
                  scale_bits == 0x40800000u && flags == 0u && enabled == 1u,
              "text conversion position and options");
        text = value;
        display_position = position;
        calls.emplace_back("display");
    }
};

std::size_t Position(const Runtime& runtime, const std::string& name) {
    for (std::size_t index = 0u; index < runtime.calls.size(); ++index) {
        if (runtime.calls[index] == name) return index;
    }
    return runtime.calls.size();
}

void CheckNormalAndFallback() {
    Runtime runtime;
    Check(pal4::MinimapSceneUiInteraction::UpdateDisplay(
              0x5000u, {0x7000u, 45.0f, 80.0f}, runtime) == 1u &&
              runtime.text == 0xd000u && runtime.rounded.size() == 2u &&
              runtime.rounded[0] == 21.0f && runtime.rounded[1] == 10.0f &&
              Position(runtime, "minimap-find") < Position(runtime, "relative") &&
              Position(runtime, "trigger-switch") < Position(runtime, "display") &&
              Position(runtime, "close") == runtime.calls.size(),
          "complete minimap trigger text path and y-before-x ftol order");

    Runtime empty;
    empty.words[0xc004u] = 0u;
    (void)pal4::MinimapSceneUiInteraction::UpdateDisplay(
        0x5000u, {0x7000u, 45.0f, 80.0f}, empty);
    Check(empty.text == 0xe000u &&
              Position(empty, "empty-string") < Position(empty, "display"),
          "null trigger text uses global empty string");
}

void CheckEarlyReturns() {
    Runtime no_minimap;
    no_minimap.minimap_object = 0u;
    Check(pal4::MinimapSceneUiInteraction::UpdateDisplay(
              0x5000u, {0x7000u, 45.0f, 80.0f}, no_minimap) == 1u &&
              no_minimap.calls.back() == "close" &&
              Position(no_minimap, "relative") == no_minimap.calls.size(),
          "missing minimap object closes hint and returns early");

    Runtime no_game_object;
    no_game_object.game_object = 0u;
    (void)pal4::MinimapSceneUiInteraction::UpdateDisplay(
        0x5000u, {0x7000u, 45.0f, 80.0f}, no_game_object);
    Check(Position(no_game_object, "trigger-object") ==
              no_game_object.calls.size() &&
              Position(no_game_object, "close") == no_game_object.calls.size(),
          "missing game object does not close hint");

    Runtime negative_trigger;
    negative_trigger.words[0xa1bcu] = 0xffffffffu;
    (void)pal4::MinimapSceneUiInteraction::UpdateDisplay(
        0x5000u, {0x7000u, 45.0f, 80.0f}, negative_trigger);
    Check(Position(negative_trigger, "trigger-object") ==
              negative_trigger.calls.size(),
          "negative signed trigger index returns before database lookup");

    Runtime no_trigger;
    no_trigger.trigger_object = 0u;
    (void)pal4::MinimapSceneUiInteraction::UpdateDisplay(
        0x5000u, {0x7000u, 45.0f, 80.0f}, no_trigger);
    Check(Position(no_trigger, "trigger-switch") == no_trigger.calls.size() &&
              Position(no_trigger, "display") == no_trigger.calls.size(),
          "missing trigger object returns without text conversion");
}

}  // namespace

int main() {
    CheckNormalAndFallback();
    CheckEarlyReturns();
}
