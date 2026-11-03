#include "pal4/smith_window.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowVisibilityRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x24u, 0x9000u},
        {object + 0x60u, 0xa000u},
        {object + 0x64u, 0xa100u},
        {object + 0x68u, 0xa200u},
        {object + 0x74u, 0xa300u},
        {object + 0x7cu, 0xa400u},
        {object + 0x44u, 0xa500u},
        {0x8b1398u, 0xb000u}, {0x8b139cu, 0xb100u},
        {0xc024u, 0xc100u}, {0xc144u, 0x9000u},
        {0xd024u, 0xd100u}, {0xd144u, 0x9000u}};
    std::map<std::uint32_t, std::int32_t> entries{
        {16u, 2}, {17u, 0}, {18u, -1}};
    std::vector<std::tuple<std::uint32_t, std::uint32_t,
                           std::uint32_t, std::uint32_t>> events;
    std::vector<std::pair<std::uint32_t, bool>> enabled;
    std::vector<std::pair<std::uint32_t, bool>> visible;
    std::vector<std::pair<std::uint32_t, bool>> top;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> removed_children;
    std::vector<std::string> calls;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    unsigned smith_reads = 0u;
    unsigned effect_calls = 0u;
    unsigned data_reads = 0u;
    unsigned active_removes = 0u;
    unsigned active_inserts = 0u;
    bool smith_active = false;
    bool dialog_visible = false;
    bool toolbar_exists = true;
    bool info_exists = true;

    void SetParent(std::uint32_t owner, std::uint8_t show,
                   std::uint32_t parent) override {
        Check(owner == object && parent == 0xe000u && show <= 1u,
              "parent and visibility arguments");
        calls.emplace_back("parent");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x5000u;
    }
    void ProcessUIEvent(std::uint32_t tree, std::uint32_t key,
                        std::uint32_t first,
                        std::uint32_t second) override {
        Check(tree == 0x5104u, "UI event tree offset");
        events.emplace_back(tree, key, first, second);
        calls.emplace_back("event");
    }
    void HandleEffect(std::uint32_t handle,
                      std::uint32_t event) override {
        Check(handle == object + 0xa4u && event == 1u,
              "effect handle and event");
        ++effect_calls;
        calls.emplace_back("effect");
    }
    bool SmithActive() override {
        ++smith_reads;
        calls.emplace_back("smith-active");
        return smith_active;
    }
    void SetSmithActive(bool active) override {
        smith_active = active;
        calls.emplace_back("clear-smith");
    }
    void SetSize(std::uint32_t window, std::uint32_t width_bits,
                 std::uint32_t height_bits) override {
        Check(window == 0x9000u, "root window size");
        width = width_bits;
        height = height_bits;
        calls.emplace_back("size");
    }
    void UpdateDisplay(std::uint32_t owner) override {
        Check(owner == object, "display update owner");
        calls.emplace_back("update-display");
    }
    std::int32_t GameDataEntry(std::uint32_t index) override {
        ++data_reads;
        calls.emplace_back("game-data");
        return entries.at(index);
    }
    void SetEnabled(std::uint32_t window, bool value) override {
        enabled.emplace_back(window, value);
        calls.emplace_back("enabled");
    }
    void SetVisible(std::uint32_t window, bool value) override {
        visible.emplace_back(window, value);
        calls.emplace_back("visible");
    }
    void SetAlwaysOnTop(std::uint32_t window,
                        bool value) override {
        top.emplace_back(window, value);
        calls.emplace_back("top");
    }
    std::uint8_t RemoveActiveFrame(std::uint32_t list,
                                   std::uint32_t adapter) override {
        Check(list == 0x5134u && adapter == object + 0x28u,
              "active frame list and adapter");
        ++active_removes;
        calls.emplace_back("remove-active");
        return 0x44u;
    }
    void InsertActiveFrame(std::uint32_t list,
                           std::uint32_t adapter) override {
        Check(list == 0x5134u && adapter == object + 0x28u,
              "active frame insert arguments");
        ++active_inserts;
        calls.emplace_back("insert-active");
    }
    std::uint8_t OnTabChange(std::uint32_t owner,
                             std::uint32_t tab) override {
        Check(owner == object && tab == 0u, "initial tab change");
        calls.emplace_back("tab");
        return 0x55u;
    }
    bool IsDialogVisible(std::uint32_t owner) override {
        Check(owner == object, "dialog visibility owner");
        calls.emplace_back("is-visible");
        return dialog_visible;
    }
    void ResetState(std::uint32_t owner) override {
        Check(owner == object, "state reset owner");
        calls.emplace_back("reset");
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                            std::uint32_t name) override {
        Check(manager == 0x5000u, "frame lookup manager");
        calls.emplace_back("find-frame");
        if (name == 0xb000u) return toolbar_exists ? 0xc000u : 0u;
        Check(name == 0xb100u, "gameInfo name pointer");
        return info_exists ? 0xd000u : 0u;
    }
    void RemoveChildWindow(std::uint32_t parent,
                           std::uint32_t child) override {
        removed_children.emplace_back(parent, child);
        calls.emplace_back("remove-child");
    }
    void ClearModelPreview(std::uint32_t owner) override {
        Check(owner == object, "save trade data owner");
        calls.emplace_back("save");
    }
};

void CheckOpeningWithRestrictions() {
    Runtime runtime;
    Check(pal4::SmithWindow::SetVisible(Runtime::object, 1u,
                                       0xe000u, runtime) == 0x55u &&
              runtime.calls.front() == "parent" &&
              runtime.events ==
                  (std::vector<std::tuple<std::uint32_t, std::uint32_t,
                                          std::uint32_t, std::uint32_t>>{
                      {0x5104u, 0x8b1394u, 1u, 0x9000u},
                      {0x5104u, 0x8b13a0u, 1u, 0x9000u},
                      {0x5104u, 0x8b13a4u, 9u, 0u}}),
          "opening publishes visibility and Smith action events");
    Check(runtime.width == 0x3f800000u &&
              runtime.height == 0x3f800000u &&
              runtime.enabled ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0xa000u, true}, {0xa100u, false},
                      {0xa200u, false}}) &&
              runtime.data_reads == 3u && runtime.smith_reads == 3u &&
              runtime.effect_calls == 1u,
          "inactive Smith state gates buttons by GameData 16-18");
    Check(runtime.visible ==
              (std::vector<std::pair<std::uint32_t, bool>>{
                  {0xa300u, true}, {0xa400u, false}}) &&
              runtime.top ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0xa500u, true}}) &&
              runtime.active_removes == 1u &&
              runtime.active_inserts == 1u &&
              runtime.calls.back() == "tab",
          "opening updates bar, mask, active list and initial tab");
}

void CheckOpeningWithSmithActive() {
    Runtime runtime;
    runtime.smith_active = true;
    runtime.words[Runtime::object + 0x74u] = 0u;
    (void)pal4::SmithWindow::SetVisible(Runtime::object, 1u,
                                        0xe000u, runtime);
    Check(runtime.height == 0x3f666666u &&
              runtime.enabled ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0xa000u, true}, {0xa100u, true},
                      {0xa200u, true}}) &&
              runtime.data_reads == 0u &&
              runtime.visible ==
                  (std::vector<std::pair<std::uint32_t, bool>>{
                      {0xa400u, false}}),
          "active Smith state uses 0.9 height and all buttons enabled");
}

void CheckClosing() {
    Runtime runtime;
    runtime.dialog_visible = true;
    runtime.smith_active = true;
    Check(pal4::SmithWindow::SetVisible(Runtime::object, 0u,
                                       0xe000u, runtime) == 0x44u &&
              runtime.events.size() == 2u &&
              std::get<2>(runtime.events[0]) == 0u &&
              runtime.effect_calls == 2u &&
              !runtime.smith_active &&
              runtime.removed_children ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x9000u, 0xc100u}, {0x9000u, 0xc100u}}) &&
              runtime.active_removes == 1u &&
              runtime.active_inserts == 0u,
          "visible close resets attachments and returns removal result");
    const auto save = std::find(runtime.calls.begin(),
                                runtime.calls.end(), "save");
    Check(save != runtime.calls.end() &&
              save + 2 == runtime.calls.end() - 1 &&
              runtime.calls.back() == "remove-active",
          "close saves data before active-list removal");

    Runtime hidden;
    hidden.smith_active = true;
    hidden.toolbar_exists = false;
    hidden.info_exists = false;
    (void)pal4::SmithWindow::SetVisible(Runtime::object, 0u,
                                        0xe000u, hidden);
    Check(hidden.smith_active && hidden.removed_children.empty() &&
              hidden.effect_calls == 2u &&
              std::find(hidden.calls.begin(), hidden.calls.end(),
                        "find-frame") == hidden.calls.end() &&
              std::find(hidden.calls.begin(), hidden.calls.end(),
                        "save") != hidden.calls.end(),
          "already hidden close skips attachment reset but saves data");
}

}  // namespace

int main() {
    try {
        CheckOpeningWithRestrictions();
        CheckOpeningWithSmithActive();
        CheckClosing();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
