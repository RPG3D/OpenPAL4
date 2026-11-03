#include "pal4/equipment_item_details.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
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

struct Runtime final : pal4::EquipmentItemDetailsRuntimeStub {
    static constexpr std::uint32_t kPlayer = 0x9000u;
    static constexpr std::uint32_t kItem = 0xa000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> strings;
    std::vector<std::pair<std::uint32_t, std::string>> texts;
    std::vector<std::string> calls;
    pal4::EquipmentImageColour colour{};
    pal4::EquipmentStatSnapshot original{};
    pal4::EquipmentStatSnapshot projected{};
    std::uint32_t player = kPlayer;
    std::uint32_t slot = 2u;
    std::int32_t level = 10;
    bool fail_projected{};

    void SetText(std::uint32_t window, std::string_view text) override {
        calls.emplace_back("text");
        texts.emplace_back(window, text);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == kPlayer + 0x928u,
              "temporary write uses classified equipment slot");
        words[address] = value;
        calls.emplace_back("write");
    }
    std::string ReadText(std::uint32_t address) override {
        calls.emplace_back("read-text");
        return strings.at(address);
    }
    std::string ConvertToUtf8(std::string_view text) override {
        calls.emplace_back("utf8");
        return std::string(text);
    }
    std::string RenderItemAttributes(std::uint32_t item) override {
        Check(item == kItem, "attribute renderer gets item");
        calls.emplace_back("render-attributes");
        return "Attributes";
    }
    std::uint32_t Player(std::uint32_t character) override {
        Check(character == 1u, "player index from window");
        calls.emplace_back("player");
        return player;
    }
    std::int32_t PlayerLevel(std::uint32_t owner,
                             std::uint32_t field) override {
        Check(owner == kPlayer && field == 4u, "level virtual field");
        calls.emplace_back("level");
        return level;
    }
    void SetImageColour(std::uint32_t image,
                        pal4::EquipmentImageColour value) override {
        Check(image == 0xe100u, "selected character marker image");
        colour = value;
        calls.emplace_back("colour");
    }
    void RefreshPlayer(std::uint32_t owner) override {
        Check(owner == kPlayer, "refresh player target");
        calls.emplace_back("refresh");
    }
    pal4::EquipmentStatSnapshot ReadStats(std::uint32_t owner) override {
        Check(owner == kPlayer, "snapshot player target");
        calls.emplace_back("stats");
        if (words.at(kPlayer + 0x928u) == 777u) {
            if (fail_projected) throw std::runtime_error("snapshot failed");
            return projected;
        }
        return original;
    }
    void InitializeProjectedStats() override {
        calls.emplace_back("initialize-projection");
    }
    std::uint32_t EquipmentSlot(std::uint32_t owner,
                                std::uint32_t item) override {
        Check(owner == kPlayer && item == kItem, "slot classifier args");
        calls.emplace_back("slot");
        return slot;
    }
    std::uint32_t RemovePreviewObject(std::uint32_t id) override {
        Check(id == 0x1234u, "clear removes saved item preview id");
        calls.emplace_back("remove-preview");
        return 0x5678u;
    }
};

pal4::EquipmentWindowState MakeState(std::uint32_t character = 1u) {
    pal4::EquipmentWindowState state{};
    state.Write(0xf4u, character);
    state.Write(0x68u, 0x6800u);
    state.Write(0x6cu, 0x6c00u);
    state.Write(0xe4u, 0xe100u);
    for (std::uint32_t index = 0u; index < 10u; ++index)
        state.Write(0x70u + index * 4u, 0x7000u + index);
    return state;
}

Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words[Runtime::kItem + 0xacu] = 0xb000u;
    runtime.words[Runtime::kItem + 0xbcu] = 0xb100u;
    runtime.words[Runtime::kItem + 0x16cu] = 5u;
    runtime.words[Runtime::kItem + 0x190u] = 777u;
    runtime.words[Runtime::kPlayer + 0x928u] = 555u;
    runtime.strings = {{0xb000u, "Sword"}, {0xb100u, "Bonus"}};
    for (std::uint32_t i = 0u; i < 84u; ++i) {
        runtime.original[i] = static_cast<std::int32_t>(100u + i);
        runtime.projected[i] = runtime.original[i];
    }
    runtime.projected[4] += 7;
    runtime.projected[5] -= 3;
    runtime.projected[31] += 2;
    return runtime;
}

void TestFormatting() {
    Check(pal4::EquipmentItemDetails::FormatStat(10, 10) == " 10",
          "unchanged stat uses plain format");
    Check(pal4::EquipmentItemDetails::FormatStat(10, 13) ==
              "<font name=systemBold> 10</font><font name=systemBold>"
              "<colour red=20 green=84 blue=208 alpha=255>+3</colour></font>",
          "positive delta uses blue format");
    Check(pal4::EquipmentItemDetails::FormatStat(10, 8) ==
              "<font name=systemBold> 10</font><font name=systemBold>"
              "<colour red=204 green=48 blue=0 alpha=255>-2</colour></font>",
          "negative delta uses red format");
}

void TestSentinelAndMissingPlayer() {
    auto state = MakeState(4u);
    Runtime runtime;
    pal4::EquipmentItemDetails::Update(state, Runtime::kItem, runtime);
    Check(runtime.texts.size() == 10u, "sentinel clears ten stat labels");
    for (std::uint32_t i = 0u; i < 10u; ++i)
        Check(runtime.texts[i] ==
                  std::pair<std::uint32_t, std::string>{0x7000u + i, ""},
              "sentinel clears correct label in order");
    Check(runtime.calls.size() == 10u,
          "sentinel avoids item and player services");

    state = MakeState();
    runtime = MakeRuntime();
    runtime.player = 0u;
    pal4::EquipmentItemDetails::Update(state, Runtime::kItem, runtime);
    Check(runtime.texts.size() == 2u &&
              runtime.texts[0].second == "Sword" &&
              runtime.texts[1].second.find("Bonus") != std::string::npos,
          "item text updates before missing player return");
    for (const auto& call : runtime.calls)
        Check(call != "stats" && call != "colour",
              "missing player skips stats and image colour");
}

void TestNullAndUnavailableItem() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    pal4::EquipmentItemDetails::Update(state, 0u, runtime);
    Check(runtime.texts.size() == 12u &&
              runtime.texts[0].second.empty() &&
              runtime.texts[1].second.empty() &&
              runtime.texts[2].second == " 104" &&
              runtime.colour.red == 1.0f,
          "null item clears details and retains baseline stats");
    for (const auto& call : runtime.calls)
        Check(call != "slot" && call != "write",
              "null item never projects equipment");

    runtime = MakeRuntime();
    runtime.words[Runtime::kItem + 0x16cu] = 20u;
    runtime.words[Runtime::kItem + 0xbcu] = 0u;
    pal4::EquipmentItemDetails::Update(state, Runtime::kItem, runtime);
    Check(runtime.colour.red > 0.57f && runtime.colour.red < 0.58f &&
              runtime.texts[1].second == "Attributes" &&
              runtime.texts[2].second == " 104",
          "level-locked item gets blue tint and no bonus text");
    for (const auto& call : runtime.calls)
        Check(call != "slot" && call != "write",
              "locked item skips projected equip");
}

void TestPreviewAndRestore() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    pal4::EquipmentItemDetails::Update(state, Runtime::kItem, runtime);
    Check(runtime.words.at(Runtime::kPlayer + 0x928u) == 555u,
          "temporary equipment restored");
    Check(runtime.colour.red == 1.0f &&
              runtime.texts.size() == 12u &&
              runtime.texts[0].second == "Sword" &&
              runtime.texts[1].second ==
                  "Attributes<br/><colour red=0 green=0 blue=0 alpha=255>"
                  "(Bonus) </colour>",
          "available item name, description and white tint");
    Check(runtime.texts[2].second.find("+7") != std::string::npos &&
              runtime.texts[3].second.find("-3") != std::string::npos &&
              runtime.texts[7].second.find("+2") != std::string::npos,
          "both stat groups show projected differences");
    const std::vector<std::string> expected_tail = {
        "slot", "read", "read", "write", "refresh", "stats",
        "write", "refresh"};
    Check(runtime.calls.size() >= expected_tail.size() + 10u,
          "preview has calls before stat labels");
    const auto start = runtime.calls.size() - 10u - expected_tail.size();
    for (std::size_t i = 0u; i < expected_tail.size(); ++i)
        Check(runtime.calls[start + i] == expected_tail[i],
              "preview saves, applies, reads and restores slot in order");

    runtime = MakeRuntime();
    runtime.slot = 6u;
    pal4::EquipmentItemDetails::Update(state, Runtime::kItem, runtime);
    for (const auto& call : runtime.calls)
        Check(call != "write", "out-of-range slot never writes");
    Check(runtime.texts[2].second == " 104",
          "out-of-range slot leaves projected stats unchanged");
}

void TestPreviewExceptionRestoresSlot() {
    auto state = MakeState();
    auto runtime = MakeRuntime();
    runtime.fail_projected = true;
    bool threw = false;
    try {
        pal4::EquipmentItemDetails::Update(state, Runtime::kItem, runtime);
    } catch (const std::runtime_error&) { threw = true; }
    Check(threw && runtime.words.at(Runtime::kPlayer + 0x928u) == 555u &&
              runtime.calls.back() == "refresh",
          "failed projected snapshot restores equipment and refreshes player");
}

void TestClearDetailsAndPreview() {
    auto state = MakeState();
    state.Write(0x98u, 0x9800u);
    for (std::uint32_t index = 0u; index < 6u; ++index)
        state.Write(0x98u + 4u * index, 0x9800u + index);
    state.Write(0xb4u, 0xb400u);
    state.Write(0xf8u, 0x1234u);
    Runtime runtime;
    Check(pal4::EquipmentItemDetails::Clear(state, runtime) == 0x5678u,
          "clear returns collection remove result");
    Check(runtime.texts.size() == 19u,
          "clear visits two headers, ten stats, six controls and footer");
    Check(runtime.texts[0].first == 0x6800u &&
              runtime.texts[1].first == 0x6c00u &&
              runtime.texts[2].first == 0x7000u &&
              runtime.texts[11].first == 0x7009u &&
              runtime.texts[12].first == 0x9800u &&
              runtime.texts[17].first == 0x9805u &&
              runtime.texts[18].first == 0xb400u,
          "clear preserves IDA control order");
    for (const auto& [window, text] : runtime.texts)
        Check(text.empty(), "clear assigns empty text");
    Check(runtime.calls.back() == "remove-preview",
          "preview removal follows all text clears");
}

}  // namespace

int main() {
    TestFormatting();
    TestSentinelAndMissingPlayer();
    TestNullAndUnavailableItem();
    TestPreviewAndRestore();
    TestPreviewExceptionRestoresSlot();
    TestClearDetailsAndPreview();
    std::cout << "Equipment item details tests passed\n";
}
