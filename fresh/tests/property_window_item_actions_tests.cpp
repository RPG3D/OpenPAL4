#include "pal4/property_window_item_actions.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyWindowItemActionsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> players{0x5000u, 0x6000u, 0u, 0x7000u};
    std::uint32_t allocation = 0xa000u;
    std::uint32_t hovered = 0x8100u;
    std::uint32_t target_type = 0u;
    std::uint32_t remaining = 0u;
    bool throw_dialog = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    std::uint32_t ObjectAddress(
        const pal4::PropertyWindowState&) override {
        calls.emplace_back("owner");
        return 0x9000u;
    }
    std::uint32_t HoveredItem(pal4::PropertyWindowState&) override {
        calls.emplace_back("hovered");
        return hovered;
    }
    void UpdateDetails(pal4::PropertyWindowState&,
                       std::uint32_t item) override {
        calls.push_back("details:" + std::to_string(item));
    }
    void SetProperty(std::uint32_t category,
                     std::uint32_t item) override {
        calls.push_back("property:" + std::to_string(category) + ":" +
                        std::to_string(item));
    }
    void ExecuteEarthballSkip(pal4::PropertyWindowState&) override {
        calls.emplace_back("earthball");
    }
    void HandleMagicItem(pal4::PropertyWindowState&) override {
        calls.emplace_back("magic");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    void ShowOrHideDialog(std::uint32_t mode, std::uint32_t callback,
                          std::uint32_t item) override {
        calls.push_back("dialog:" + std::to_string(mode) + ":" +
                        std::to_string(callback) + ":" +
                        std::to_string(item));
        if (throw_dialog) throw std::runtime_error("dialog");
    }
    void ReleaseCallback(std::uint32_t callback,
                         std::uint32_t flag) override {
        calls.push_back("release:" + std::to_string(callback) + ":" +
                        std::to_string(flag));
    }
    void PlayMusic(std::string_view file, std::uint32_t mode) override {
        Check(file == "WD001.WAV" && mode == 0u,
              "use action plays exact source sound");
        calls.emplace_back("music");
    }
    std::uint32_t ItemTargetType(std::uint32_t item) override {
        Check(item == 0x8100u, "callback type reads chosen item");
        calls.emplace_back("target-type");
        return target_type;
    }
    std::uint32_t PlayerByIndex(std::uint32_t index) override {
        calls.push_back("player:" + std::to_string(index));
        return index < players.size() ? players[index] : 0u;
    }
    void ApplyItemEffect(std::uint32_t caster, std::uint32_t effect,
                         std::uint32_t target) override {
        calls.push_back("effect:" + std::to_string(caster) + ":" +
                        std::to_string(effect) + ":" +
                        std::to_string(target));
    }
    std::uint32_t RemoveItem(std::uint32_t item_id,
                             std::uint32_t amount) override {
        Check(item_id == 123u && amount == 1u,
              "removal receives definition ID and one item");
        calls.emplace_back("remove");
        return remaining;
    }
    void PopulateList(pal4::PropertyWindowState&,
                      std::uint32_t category,
                      bool preserve_scroll) override {
        Check(category == 5u && !preserve_scroll,
              "use action refreshes current category without scroll");
        calls.emplace_back("populate");
    }
};

void SetupItem(Runtime& runtime, std::uint32_t id) {
    runtime.words[0x7004u] = 0x8100u;
    runtime.words[0x8100u] = 0x8200u;
    runtime.words[0x8200u] = id;
    runtime.words[0x8104u] = 0x8300u;
}

void TestHoverAndClick() {
    pal4::PropertyWindowState state;
    state.Write(108u, 5u);
    Runtime runtime;
    Check(pal4::PropertyWindowItemActions::HandleHover(state, runtime),
          "hover callback returns true");
    Check(runtime.calls == std::vector<std::string>{
              "hovered", "details:33024"},
          "hover passes resolved item to details updater");

    runtime.calls.clear();
    runtime.words[0x7004u] = 0u;
    Check(pal4::PropertyWindowItemActions::ProcessClick(
              state, 0x7000u, runtime), "null click returns true");
    Check(runtime.calls.empty(), "null item has no side effects");

    SetupItem(runtime, 3067u);
    Check(pal4::PropertyWindowItemActions::ProcessClick(
              state, 0x7000u, runtime), "earthball click returns true");
    Check(runtime.calls == std::vector<std::string>{
              "property:5:33536", "earthball"},
          "earthball stores next item before skip script");

    runtime.calls.clear();
    runtime.words[0x8200u] = 3182u;
    Check(pal4::PropertyWindowItemActions::ProcessClick(
              state, 0x7000u, runtime), "magic-item click returns true");
    Check(runtime.calls == std::vector<std::string>{"magic"},
          "magic item follows separate use path");

    runtime.calls.clear();
    runtime.words[0x8200u] = 123u;
    Check(pal4::PropertyWindowItemActions::ProcessClick(
              state, 0x7000u, runtime), "ordinary click returns true");
    Check(runtime.words[0xa000u] ==
              pal4::PropertyWindowItemActions::kCallbackVtable &&
          runtime.words[0xa008u] ==
              pal4::PropertyWindowItemActions::kUseCallback &&
          runtime.words[0xa00cu] == 0u &&
          runtime.words[0xa010u] == 0x9000u,
          "ordinary click writes callback vtable, handler and owner");
    Check(runtime.calls.front() == "allocate:24" &&
          runtime.calls[runtime.calls.size() - 2u] ==
              "dialog:1:40960:33024" &&
          runtime.calls.back() == "release:40960:1",
          "ordinary click shows dialog and releases local callback");

    runtime.calls.clear();
    runtime.allocation = 0u;
    Check(pal4::PropertyWindowItemActions::ProcessClick(
              state, 0x7000u, runtime), "failed allocation returns true");
    Check(runtime.calls == std::vector<std::string>{
              "allocate:24", "dialog:1:0:33024"},
          "failed allocation still invokes dialog with null callback");

    runtime.calls.clear();
    runtime.allocation = 0xa000u;
    runtime.throw_dialog = true;
    try {
        (void)pal4::PropertyWindowItemActions::ProcessClick(
            state, 0x7000u, runtime);
        Check(false, "dialog exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "release:40960:1",
              "dialog exception releases callback ownership");
    }
}

void TestUseItem() {
    pal4::PropertyWindowState state;
    state.Write(108u, 5u);
    Runtime runtime;
    runtime.words[0x9000u] = 0u;
    runtime.words[0x9004u] = 0x9100u;
    runtime.words[0x9100u] = 0x8100u;
    runtime.words[0x8100u] = 123u;
    runtime.words[0x8194u] = 77u;

    runtime.players[0] = 0u;
    Check(pal4::PropertyWindowItemActions::UseItem(
              state, 0x9000u, runtime), "missing player returns true");
    Check(runtime.calls == std::vector<std::string>{
              "music", "target-type", "player:0"},
          "missing player stops after sound and lookup");

    runtime.calls.clear();
    runtime.players[0] = 0x5000u;
    Check(pal4::PropertyWindowItemActions::UseItem(
              state, 0x9000u, runtime), "single-target use returns true");
    Check(runtime.calls == std::vector<std::string>{
              "music", "target-type", "player:0",
              "effect:22100:77:22100", "remove", "property:5:0",
              "dialog:0:0:0", "populate"},
          "single target applies effect then removes final item and closes dialog");

    runtime.calls.clear();
    runtime.target_type = 3u;
    runtime.remaining = 0xa100u;
    runtime.words[0xa104u] = 0x8300u;
    runtime.words[0x5000u + 0xb08u] = 1u;
    runtime.words[0x6000u + 0xb08u] = 0u;
    runtime.words[0x7000u + 0xb08u] = 1u;
    Check(pal4::PropertyWindowItemActions::UseItem(
              state, 0x9000u, runtime), "party use returns true");
    Check(runtime.calls == std::vector<std::string>{
              "music", "target-type", "player:0", "player:0",
              "effect:22100:77:22100", "player:1", "player:2",
              "player:3", "effect:22100:77:30292", "remove",
              "property:5:33536", "populate"},
          "party use filters four slots and keeps next inventory item");
}

}  // namespace

int main() {
    TestHoverAndClick();
    TestUseItem();
}
