#include "pal4/equipment_character_display.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::EquipmentCharacterDisplayRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::string>> images;
    std::vector<std::pair<std::uint32_t, std::string>> texts;
    std::vector<std::pair<std::uint32_t, bool>> enabled;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint32_t> items;
    std::unordered_map<std::uint32_t, std::string> names;
    std::uint32_t player = 0x9000u;

    void SetImage(std::uint32_t window, std::string_view imageset,
                  std::string_view image) override {
        Check(window == 0x4400u && imageset == "zi0", "portrait target");
        calls.emplace_back("image");
        images.emplace_back(window, std::string(image));
    }
    std::uint32_t Player(std::uint32_t character) override {
        Check(character == 1u, "equipment character");
        calls.emplace_back("player");
        return player;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t InventoryItem(std::uint32_t item_id,
                                std::uint32_t kind) override {
        Check(kind == 7u, "inventory record kind");
        calls.emplace_back("item");
        const auto it = items.find(item_id);
        return it == items.end() ? 0u : it->second;
    }
    std::string ItemText(std::uint32_t address) override {
        calls.emplace_back("name");
        return names.at(address);
    }
    void SetText(std::uint32_t window, std::string_view text) override {
        calls.emplace_back("text");
        texts.emplace_back(window, text);
    }
    void SetEnabled(std::uint32_t window, bool value) override {
        calls.emplace_back("enabled");
        enabled.emplace_back(window, value);
    }
};

pal4::EquipmentWindowState MakeState() {
    pal4::EquipmentWindowState state{};
    state.Write(0x44u, 0x4400u);
    for (std::uint32_t i = 0u; i < 6u; ++i)
        state.Write(0x98u + i * 4u, 0x1000u + i * 0x200u);
    return state;
}

Runtime MakeRuntime() {
    Runtime runtime;
    for (std::uint32_t i = 0u; i < 6u; ++i) {
        const auto window = 0x1000u + i * 0x200u;
        runtime.words[window + 0x118u] = i;
        runtime.words[runtime.player + 0x920u + i * 4u] = 11u + i;
    }
    runtime.words[0x1600u + 0x118u] = 99u;
    runtime.items = {{11u, 0x8000u}, {12u, 0x8100u},
                     {15u, 0x8200u}, {99u, 0x8300u}};
    runtime.words[0x8000u + 0x9cu] = 0x8800u;
    runtime.words[0x8100u + 0x9cu] = 0x8900u;
    runtime.words[0x8200u + 0x9cu] = 0u;
    runtime.words[0x8300u + 0x9cu] = 0x8b00u;
    runtime.names = {{0x8800u, "Sword"}, {0x8900u, "Hat"},
                     {0x8b00u, "Direct"}};
    return runtime;
}

void TestPortrait() {
    auto state = MakeState();
    Runtime runtime;
    const std::vector<std::string> expected = {
        "yuntianhe", "hanlingsha", "liumengli", "murongziying"};
    for (std::uint32_t i = 0u; i < 4u; ++i)
        pal4::EquipmentCharacterDisplay::SetCharacterImage(state, i, runtime);
    Check(runtime.images.size() == 4u, "four portrait images");
    for (std::size_t i = 0u; i < expected.size(); ++i)
        Check(runtime.images[i].second == expected[i],
              "portrait name follows character index");
    pal4::EquipmentCharacterDisplay::SetCharacterImage(state, 4u, runtime);
    pal4::EquipmentCharacterDisplay::SetCharacterImage(state, 99u, runtime);
    Check(runtime.images.size() == 4u, "invalid index has no image call");
}

void TestEquipment() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    pal4::EquipmentCharacterDisplay::UpdateEquipment(state, 1u, runtime);
    Check(runtime.texts.size() == 6u && runtime.enabled.size() == 6u,
          "six equipment controls updated");
    Check(runtime.texts[0] == std::pair<std::uint32_t, std::string>{
              0x1000u, "Sword"} && !runtime.enabled[0].second,
          "first equipment remains disabled even when present");
    Check(runtime.texts[1].second == "Hat" && runtime.enabled[1].second,
          "present later equipment is enabled");
    Check(runtime.texts[2].second.empty() && !runtime.enabled[2].second,
          "missing inventory record clears control");
    Check(runtime.texts[3].second == "Direct" &&
              runtime.enabled[3].second,
          "out-of-range window property is direct inventory id");
    Check(runtime.texts[4].second.empty() && runtime.enabled[4].second,
          "null name becomes empty but item remains enabled");
    Check(runtime.calls.front() == "player" &&
              runtime.calls.back() == "enabled",
          "player lookup precedes six control updates");
}

void TestSentinelAndMissingPlayer() {
    auto state = MakeState();
    Runtime runtime;
    pal4::EquipmentCharacterDisplay::UpdateEquipment(state, 4u, runtime);
    Check(runtime.texts.size() == 6u && runtime.enabled.size() == 6u,
          "sentinel clears six controls");
    for (std::size_t i = 0u; i < 6u; ++i)
        Check(runtime.texts[i].second.empty() &&
                  !runtime.enabled[i].second,
              "sentinel text and enabled state");
    for (const auto& call : runtime.calls)
        Check(call != "player" && call != "item",
              "sentinel skips player and inventory");

    runtime = Runtime{};
    runtime.player = 0u;
    pal4::EquipmentCharacterDisplay::UpdateEquipment(state, 1u, runtime);
    Check(runtime.calls == std::vector<std::string>{"player"},
          "missing player leaves controls untouched");
}

}  // namespace

int main() {
    TestPortrait();
    TestEquipment();
    TestSentinelAndMissingPlayer();
    std::cout << "Equipment character display tests passed\n";
}
