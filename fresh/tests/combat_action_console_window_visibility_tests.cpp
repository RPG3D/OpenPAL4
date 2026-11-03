#include "pal4/combat_action_console_window_visibility.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct ArrayRuntime final : pal4::IEventHandlerArrayRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x4138u, 0x9000u}, {0x413cu, 0x9000u}, {0x4140u, 0x9010u}};
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t) override {
        throw std::runtime_error("unexpected array growth");
    }
    void Free(std::uint32_t) override {
        throw std::runtime_error("unexpected array free");
    }
    std::uint32_t InvokeVirtual(std::uint32_t object,
                                std::uint32_t slot) override {
        Check(object == 0x1028u && slot == 0xcu,
              "active handler array invokes navigation slot 0xc");
        calls.emplace_back("invoke");
        return 0x77u;
    }
    std::uint8_t InvokePredicate(std::uint32_t, std::uint32_t,
                                 std::uint32_t) override {
        return 0u;
    }
};

struct Runtime final : pal4::ICombatActionConsoleWindowVisibilityRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::uint8_t> enabled;
    std::vector<std::uint32_t> actors{0x8000u, 0x8100u};
    std::vector<std::string> calls;
    std::uint32_t iterator_index{};
    std::uint32_t redraws{};
    std::uint32_t releases{};
    std::uint8_t panel_visible{};
    std::uint32_t disabled_mask = 0x3u;
    bool found_magic = true;
    bool throw_magic{};

    Runtime() {
        for (std::uint32_t index = 0u; index < 6u; ++index)
            words[0x1150u + 4u * index] = 0x7000u + 4u * index;
        words[0x11e0u] = 0xa000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    std::uint8_t ActorProperty(std::uint32_t context,
                               std::uint32_t index) override {
        Check(context == 0x5000u && index < 6u,
              "actor property context and index");
        calls.emplace_back("property:" + std::to_string(index));
        return 1u;
    }
    std::uint32_t ActorManager() override {
        calls.emplace_back("actor-manager");
        return 0x6000u;
    }
    std::uint32_t AcquireActorIterator(std::uint32_t collection) override {
        Check(collection == 0x6020u, "actor iterator collection");
        calls.emplace_back("acquire");
        return 0x6200u;
    }
    bool IteratorAtEnd(std::uint32_t iterator) override {
        Check(iterator == 0x6200u, "iterator end handle");
        calls.emplace_back("end");
        return iterator_index >= actors.size();
    }
    std::uint32_t IteratorCurrent(std::uint32_t iterator) override {
        Check(iterator == 0x6200u, "iterator current handle");
        calls.emplace_back("current");
        return actors.at(iterator_index);
    }
    void IteratorNext(std::uint32_t iterator) override {
        Check(iterator == 0x6200u, "iterator next handle");
        calls.emplace_back("next");
        ++iterator_index;
    }
    bool ActorHasMagic(std::uint32_t collection) override {
        Check(collection == actors.at(iterator_index) + 0x398u,
              "actor magic collection offset");
        calls.emplace_back("magic");
        if (throw_magic) throw std::runtime_error("magic");
        return found_magic && iterator_index == 1u;
    }
    void ReleaseIterator(std::uint32_t iterator,
                         std::uint32_t flags) override {
        Check(iterator == 0x6200u && flags == 1u,
              "iterator deleting release");
        ++releases;
        calls.emplace_back("release");
    }
    void SetEnabled(std::uint32_t button,
                    std::uint8_t value) override {
        enabled[button] = value;
        calls.emplace_back("enabled:" + std::to_string(button));
    }
    void RequestRedraw(std::uint32_t) override {
        ++redraws;
        calls.emplace_back("redraw");
    }
    bool IsDisabled(std::uint32_t button,
                    bool recursive) override {
        Check(!recursive, "nonrecursive disabled query");
        const auto index = (button - 0x7000u) / 4u;
        return (disabled_mask & (1u << index)) != 0u ||
               enabled[button] == 0u;
    }
    void SetVisible(std::uint32_t panel,
                    std::uint8_t value) override {
        Check(panel == 0xa000u, "control panel pointer");
        panel_visible = value;
        calls.emplace_back("visible");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("frame-manager");
        return 0x4000u;
    }
};

void CheckVisibleThenHidden() {
    Runtime runtime;
    ArrayRuntime array;
    Check(pal4::CombatActionConsoleWindowVisibility::Update(
              0x1000u, 0x5000u, 1u, runtime, array) == 0x77u,
          "visible path forwards event array insert result");
    Check(runtime.releases == 1u && runtime.iterator_index == 1u &&
              runtime.enabled[0x700cu] == 1u,
          "magic gate finds the second actor and releases iterator");
    Check(runtime.words[0x103cu] == 2u &&
              runtime.words[0x11e4u] == 0x1190u &&
              runtime.bytes[0x7008u + 0x4b1u] == 1u &&
              runtime.redraws == 7u,
          "first available button is selected after all six redraws");
    Check(runtime.panel_visible == 1u &&
              array.words[0x9000u] == 0x1028u &&
              array.words[0x413cu] == 0x9004u,
          "visible panel adds navigation to manager array at +0x134");
    runtime.calls.clear();
    Check(pal4::CombatActionConsoleWindowVisibility::Update(
              0x1000u, 0u, 0u, runtime, array) == 0u &&
              runtime.panel_visible == 0u &&
              array.words[0x413cu] == 0x9000u,
          "IDA4AED70 returns remaining count zero when removing the sole navigation handler");
    for (std::uint32_t index = 0u; index < 6u; ++index)
        Check(runtime.enabled[0x7000u + 4u * index] == 0u,
              "hidden path disables each button");
    Check(runtime.calls.size() == 8u &&
              runtime.calls[6] == "visible" &&
              runtime.calls[7] == "frame-manager",
          "hidden path skips actor queries and redraws");
}

void CheckNoAvailableButton() {
    Runtime runtime;
    runtime.actors.clear();
    runtime.disabled_mask = 0x3fu;
    runtime.words[0x103cu] = 0xabu;
    runtime.words[0x11e4u] = 0xcdefu;
    ArrayRuntime array;
    Check(pal4::CombatActionConsoleWindowVisibility::Update(
              0x1000u, 0x5000u, 1u, runtime, array) == 0x77u &&
              runtime.enabled[0x700cu] == 0u &&
              runtime.words[0x103cu] == 0xabu &&
              runtime.words[0x11e4u] == 0xcdefu &&
              runtime.redraws == 6u && runtime.releases == 1u,
          "empty actor iterator disables magic and all-disabled scan retains selection");
}

void CheckIteratorException() {
    Runtime runtime;
    runtime.throw_magic = true;
    ArrayRuntime array;
    bool caught = false;
    try {
        (void)pal4::CombatActionConsoleWindowVisibility::Update(
            0x1000u, 0x5000u, 1u, runtime, array);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.releases == 1u &&
              runtime.calls.back() == "release" &&
              array.words[0x413cu] == 0x9000u,
          "magic query exception releases iterator before panel registration");
}

}  // namespace

int main() {
    try {
        CheckVisibleThenHidden();
        CheckNoAvailableButton();
        CheckIteratorException();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
