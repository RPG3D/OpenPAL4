#include "pal4/combat_console.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kConsole = 0x5000u;
constexpr std::uint32_t kRoot = 0x7000u;
constexpr std::uint32_t kCallback = 0xc000u;

constexpr std::array<std::string_view, 5> kFixedNames{{
    "CombatMainWindow/StaticActionSequence",
    "CombatMainWindow/ImgJointAttackHint",
    "CombatMainWindow/guaishuliang",
    "CombatMainWindow/PanelMonsterInfo",
    "CombatMainWindow/EditMonsterInfo",
}};

struct Runtime final : pal4::ICombatConsoleMainWindowRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {kConsole + 36u, kRoot}};
    std::vector<std::string> calls;
    std::vector<std::string> dynamic_names;
    std::vector<std::uint32_t> hidden;
    std::uint32_t fixed_index = 0u;
    std::uint32_t dynamic_index = 0u;
    std::uint32_t converted = 0u;
    std::uint32_t allocation = kCallback;
    std::uint32_t released = 0u;
    bool transfer = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kConsole + 36u, "root field read");
        calls.emplace_back("root");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        if (address == kCallback + 8u || address == kCallback + 12u ||
            address == kCallback + 16u || address == kCallback)
            calls.emplace_back("callback-field");
    }
    std::uint32_t FindWindowByName(std::string_view name) override {
        if (fixed_index == 0u) {
            Check(name == kFixedNames[0], "main window name");
            ++fixed_index;
            calls.emplace_back("main");
            return 0x8000u;
        }
        dynamic_names.emplace_back(name);
        const auto value = 0x9000u + dynamic_index++ * 0x100u;
        calls.emplace_back("dynamic");
        return value;
    }
    std::uint32_t FindWindow(std::uint32_t console,
                             std::string_view name) override {
        Check(console == kConsole && fixed_index < kFixedNames.size() &&
                  name == kFixedNames[fixed_index],
              "fixed controls found in IDA order");
        const auto value = 0x8100u + fixed_index++ * 0x100u;
        calls.emplace_back("fixed");
        return value;
    }
    void SetAutoBreak(std::uint32_t edit, bool enabled) override {
        Check(edit == 0x8500u && !enabled,
              "monster info edit auto-break disabled");
        calls.emplace_back("auto-break");
    }
    std::string ConvertToUtf8(std::string_view source) override {
        ++converted;
        calls.emplace_back("convert");
        return std::string(source);
    }
    void SetChildVisible(std::uint32_t child, bool visible) override {
        Check(!visible, "dynamic images start hidden");
        hidden.push_back(child);
        calls.emplace_back("hidden");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 24u, "mouse callback allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t MouseClickEvent() override {
        calls.emplace_back("mouse-event");
        return 0xe000u;
    }
    void SubscribeClick(std::uint32_t root, std::uint32_t event,
                        std::uint32_t& callback) override {
        Check(root == kRoot && event == 0xe000u &&
                  callback == allocation,
              "click subscription target and callback");
        if (transfer) callback = 0u;
        calls.emplace_back("subscribe");
    }
    void ReleaseCallback(std::uint32_t callback) override {
        Check(callback == kCallback, "untransferred callback release");
        ++released;
        calls.emplace_back("release");
    }
};

std::size_t FindCall(const Runtime& runtime, const std::string& name) {
    for (std::size_t index = 0; index < runtime.calls.size(); ++index)
        if (runtime.calls[index] == name) return index;
    throw std::runtime_error("expected call absent");
}

void CheckControls(const Runtime& runtime) {
    Check(runtime.fixed_index == 5u && runtime.dynamic_index == 9u &&
              runtime.converted == 9u && runtime.hidden.size() == 9u,
          "five fixed and nine converted dynamic controls");
    Check(runtime.words.at(kConsole + 212u) == 0x8000u &&
              runtime.words.at(kConsole + 216u) == 0x8200u &&
              runtime.words.at(kConsole + 220u) == 0x8300u &&
              runtime.words.at(kConsole + 260u) == 0x8400u &&
              runtime.words.at(kConsole + 264u) == 0x8500u,
          "fixed control offsets");
    for (std::uint32_t index = 0u; index < 9u; ++index) {
        const auto expected = std::string("CombatMainWindow/Static") +
                              (index < 3u ? "Role" : "Monster") +
                              std::to_string(index < 3u ? index + 1u
                                                        : index - 2u);
        const auto image = 0x9000u + index * 0x100u;
        const auto slot = kConsole + (index < 3u ? 224u + index * 4u
                                                : 236u + (index - 3u) * 4u);
        Check(runtime.dynamic_names[index] == expected &&
                  runtime.words.at(slot) == image &&
                  runtime.hidden[index] == image &&
                  runtime.words.at(image + 280u) == 0u,
              "dynamic image name, slot and initial state");
    }
    Check(FindCall(runtime, "main") < FindCall(runtime, "auto-break") &&
              FindCall(runtime, "auto-break") < FindCall(runtime, "convert") &&
              FindCall(runtime, "hidden") < FindCall(runtime, "allocate") &&
              FindCall(runtime, "allocate") < FindCall(runtime, "subscribe"),
          "window, image and callback phase order");
}

void CallbackOwnershipBranches() {
    for (const bool transfer : {false, true}) {
        Runtime runtime;
        runtime.transfer = transfer;
        Check(pal4::CombatConsoleWindow::InitializeMainWindow(
                  kConsole, runtime) == 0u,
              "initializer defines temporary cleanup result as zero");
        CheckControls(runtime);
        Check(runtime.words.at(kCallback) ==
                  pal4::CombatConsoleWindow::kClickCallbackVtable &&
                  runtime.words.at(kCallback + 8u) ==
                      pal4::CombatConsoleWindow::kClickCallback &&
                  runtime.words.at(kCallback + 12u) == 0u &&
                  runtime.words.at(kCallback + 16u) == kConsole &&
                  runtime.released == (transfer ? 0u : 1u),
              "callback fields and ownership transfer");
    }
    Runtime no_allocation;
    no_allocation.allocation = 0u;
    (void)pal4::CombatConsoleWindow::InitializeMainWindow(
        kConsole, no_allocation);
    CheckControls(no_allocation);
    Check(no_allocation.released == 0u &&
              no_allocation.words.count(kCallback) == 0u &&
              no_allocation.calls.back() == "subscribe",
          "allocation failure still subscribes a null callback");
}

}  // namespace

int main() {
    try {
        CallbackOwnershipBranches();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
