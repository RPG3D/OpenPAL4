#include "pal4/property_window_selection.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyWindowSelectionRuntime {
    std::vector<std::string> calls;
    std::optional<pal4::PropertyHoveredItem> hovered;
    bool component_ok = true;
    std::vector<pal4::PropertySpecialEntry> properties;

    std::uint32_t Dialog() override {
        calls.emplace_back("dialog");
        return 0x7000u;
    }
    void HideAndCleanup(std::uint32_t dialog) override {
        Check(dialog == 0x7000u, "existing choice dialog is cleaned");
        calls.emplace_back("hide");
    }
    std::optional<pal4::PropertyHoveredItem> HoveredItem(
        pal4::PropertyWindowState&) override {
        calls.emplace_back("hovered");
        return hovered;
    }
    bool CheckComponent(std::uint32_t definition) override {
        Check(definition == 0x8100u,
              "use gate receives item definition pointer");
        calls.emplace_back("component");
        return component_ok;
    }
    std::string ArenaObjectName() override {
        calls.emplace_back("arena-object");
        return "object";
    }
    std::string ArenaSceneName() override {
        calls.emplace_back("arena-scene");
        return "scene";
    }
    std::string BuildSpecialKey(std::string_view scene,
                                std::string_view object) override {
        Check(scene == "scene" && object == "object",
              "special key combines arena scene and object in call order");
        calls.emplace_back("special-key");
        return "scene/object";
    }
    std::vector<pal4::PropertySpecialEntry> SpecialProperties() override {
        calls.emplace_back("properties");
        return properties;
    }
    bool KeysEqual(std::string_view left,
                   std::string_view right) override {
        Check(left == "scene/object", "property lookup uses special key");
        calls.push_back("compare:" + std::string(right));
        return left == right;
    }
    std::string TriggerText(std::uint32_t trigger) override {
        calls.push_back("trigger:" + std::to_string(trigger));
        return trigger == 22u ? "Use" : "Discard";
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.push_back("utf8:" + std::string(text));
        return "utf8:" + std::string(text);
    }
    void CreateButton(std::uint32_t dialog, std::string_view action,
                      std::uint32_t item, std::string_view label,
                      bool enabled) override {
        Check(dialog == 0x7000u && item == 0x8000u,
              "button callback stores hovered item");
        calls.push_back("button:" + std::string(action) + ":" +
                        std::string(label) + (enabled ? ":1" : ":0"));
    }
    void ShowDialog(std::uint32_t dialog,
                    std::uint32_t event_data) override {
        Check(dialog == 0x7000u && event_data == 0x9000u,
              "choice dialog receives event payload");
        calls.emplace_back("show");
    }
};

void TestMissingAndOrdinary() {
    pal4::PropertyWindowState state;
    Runtime runtime;
    pal4::PropertyWindowSelection::ShowChoices(
        state, 0x9000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "dialog", "hide", "hovered"},
          "missing item still clears prior dialog but creates no buttons");

    runtime = Runtime{};
    runtime.hovered = {0x8000u, 0x8100u, 42u, 4u};
    pal4::PropertyWindowSelection::ShowChoices(
        state, 0x9000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "dialog", "hide", "hovered", "component",
              "trigger:22", "utf8:Use",
              "button:use_property:utf8:Use:1",
              "trigger:23", "utf8:Discard",
              "button:discard_property:utf8:Discard:0", "show"},
          "ordinary type-four item may be used but cannot be discarded");
    runtime = Runtime{};
    runtime.hovered = {0x8000u, 0x8100u, 42u, 1u};
    runtime.component_ok = false;
    pal4::PropertyWindowSelection::ShowChoices(
        state, 0x9000u, runtime);
    Check(runtime.calls[6] == "button:use_property:utf8:Use:0" &&
              runtime.calls[9] ==
                  "button:discard_property:utf8:Discard:1",
          "component gate disables use without disabling discard");
}

void TestSpecialItem() {
    pal4::PropertyWindowState state;
    Runtime runtime;
    runtime.hovered = {0x8000u, 0x8100u, 3067u, 1u};
    runtime.properties = {{"other", 10}, {"scene/object", 0},
                          {"scene/object", 5}};
    pal4::PropertyWindowSelection::ShowChoices(
        state, 0x9000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "dialog", "hide", "hovered", "component",
              "arena-object", "arena-scene", "special-key", "properties",
              "compare:other", "compare:scene/object",
              "compare:scene/object", "trigger:22", "utf8:Use",
              "button:use_property:utf8:Use:0", "trigger:23",
              "utf8:Discard", "button:discard_property:utf8:Discard:0",
              "show"},
          "special item uses first matching signed Scene EARTHBALL and forbids discard");
    runtime = Runtime{};
    runtime.hovered = {0x8000u, 0x8100u, 3067u, 1u};
    runtime.properties = {{"scene/object", 1}};
    pal4::PropertyWindowSelection::ShowChoices(
        state, 0x9000u, runtime);
    Check(runtime.calls[11] == "button:use_property:utf8:Use:1",
          "positive first match enables special item use");
    runtime = Runtime{};
    runtime.hovered = {0x8000u, 0x8100u, 3067u, 1u};
    runtime.component_ok = false;
    pal4::PropertyWindowSelection::ShowChoices(
        state, 0x9000u, runtime);
    Check(runtime.calls.size() == 11u &&
              runtime.calls[4] == "trigger:22" &&
              runtime.calls[6] == "button:use_property:utf8:Use:0",
          "failed component gate skips arena/database special lookup");
}

}  // namespace

int main() {
    TestMissingAndOrdinary();
    TestSpecialItem();
    std::cout << "Property window selection tests passed\n";
}
