#include "pal4/trade_window_player_slot.h"

#include <algorithm>
#include <array>
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

struct Visibility {
    std::uint32_t window;
    bool visible;
};

struct Runtime final : pal4::ITradeWindowPlayerSlotRuntime {
    static constexpr std::uint32_t slot = 0x1000u;
    static constexpr std::uint32_t player = 0x3000u;
    static constexpr std::uint32_t item = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<Visibility> visibility;
    std::map<std::uint32_t, std::string> text;
    std::vector<std::uint32_t> queried_ids;
    std::vector<std::uint32_t> included;
    std::vector<pal4::TradeSlotColour> colours;
    std::array<std::int32_t, 5> updated{15, 20, 25, 40, 60};
    std::uint32_t found_player = player;
    std::int32_t stat = 10;
    std::int32_t excluded = 2;
    bool throw_finish = false;
    std::uint32_t result = 0xabcd1234u;

    Runtime() {
        for (std::uint32_t index = 0u; index < 8u; ++index)
            words[slot + index * 4u] = 0x2000u + index;
        words[player + 0xb08u] = 1u;
        words[item + 0x168u + 4u] = 5u;
        for (std::uint32_t index = 0u; index < 5u; ++index)
            words[player + 0x7b8u + index * 4u] = 10u * (index + 1u);
        for (std::uint32_t index = 0u; index < 6u; ++index)
            words[player + 0x920u + index * 4u] = 100u + index;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x2000u + 0x6b4u && value == 0u,
              "icon state byte");
        calls.emplace_back("clear-icon-state");
    }
    std::uint32_t PlayerByIndex(std::uint32_t index) override {
        Check(index == 1u, "player index");
        calls.emplace_back("player");
        return found_player;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        calls.emplace_back("visible");
        visibility.push_back({window, visible});
    }
    std::uint32_t IconImage(std::uint32_t index,
                            std::uint32_t type,
                            std::uint32_t variant) override {
        Check(index == 1u && type == 2u && variant == 0u,
              "icon lookup arguments");
        calls.emplace_back("icon-lookup");
        return 0x9000u;
    }
    void SetIconImage(std::uint32_t window,
                      std::uint32_t image) override {
        Check(window == 0x2000u && image == 0x9000u,
              "icon image target");
        calls.emplace_back("icon-image");
    }
    std::int32_t PlayerStat(std::uint32_t owner,
                            std::uint32_t kind) override {
        Check(owner == player && kind == 4u, "player stat query");
        calls.emplace_back("stat");
        return stat;
    }
    std::string ConvertToUtf8(std::string_view source) override {
        calls.emplace_back("utf8");
        return std::string(source);
    }
    void SetText(std::uint32_t window,
                 std::string_view value) override {
        calls.emplace_back("text");
        text[window] = value;
    }
    void SetIconColour(std::uint32_t window,
                       pal4::TradeSlotColour colour) override {
        Check(window == 0x2000u, "icon colour target");
        calls.emplace_back("colour");
        colours.push_back(colour);
    }
    std::int32_t ClassifySlot(std::uint32_t equipment,
                              std::uint32_t value) override {
        Check(equipment == player + 0x920u && value == item,
              "equipment classifier arguments");
        calls.emplace_back("classify");
        return excluded;
    }
    void RefreshEquipment(std::uint32_t component) override {
        Check(component == player + 0x654u,
              "equipment component");
        calls.emplace_back("refresh");
    }
    void StartPreview(std::uint32_t value) override {
        Check(value == item, "preview candidate");
        calls.emplace_back("start-preview");
    }
    std::uint32_t InventoryBase() override {
        calls.emplace_back("inventory-base");
        return 0x7000u;
    }
    std::uint32_t FindInventoryItem(std::uint32_t collection,
                                    std::uint32_t id,
                                    std::uint32_t category) override {
        Check(collection == 0x707cu && category == 7u,
              "equipped item search arguments");
        calls.emplace_back("find-inventory");
        queried_ids.push_back(id);
        return id == 101u ? 0u : id + 0x1000u;
    }
    void AddPreviewRecord(std::uint32_t record) override {
        calls.emplace_back("add-preview");
        included.push_back(record);
    }
    std::array<std::int32_t, 5> FinishPreview(
        std::uint32_t owner) override {
        Check(owner == player, "preview player");
        calls.emplace_back("finish-preview");
        if (throw_finish) throw std::runtime_error("snapshot failed");
        return updated;
    }
    void CleanupPreview() override {
        calls.emplace_back("cleanup-preview");
    }
    std::uint32_t CleanupTextBuffers() override {
        calls.emplace_back("cleanup-text");
        return result;
    }
};

void CheckQualified() {
    Runtime runtime;
    Check(pal4::TradeWindowPlayerSlot::Update(
              Runtime::slot, 1u, Runtime::item, runtime) == runtime.result,
          "qualified result");
    Check(runtime.visibility.size() == 7u, "seven visible controls");
    constexpr std::array<std::uint32_t, 7> order{
        0x2002u, 0x2003u, 0x2004u, 0x2005u,
        0x2006u, 0x2000u, 0x2001u};
    for (std::size_t index = 0; index < order.size(); ++index)
        Check(runtime.visibility[index].window == order[index] &&
              runtime.visibility[index].visible,
              "initial visibility order");
    Check(runtime.queried_ids == std::vector<std::uint32_t>{
              100u, 101u, 103u, 104u, 105u},
          "six equipment positions exclude replacement slot");
    Check(runtime.included == std::vector<std::uint32_t>{
              0x1064u, 0x1067u, 0x1068u, 0x1069u},
          "missing equipped item skipped");
    Check(runtime.text.at(0x2007u) == "10" &&
          runtime.text.at(0x2002u) ==
              "<font name=systemBold><colour red=20 green=84 blue=208 alpha=255>+5</colour></font>" &&
          runtime.text.at(0x2003u).empty() &&
          runtime.text.at(0x2004u) ==
              "<font name=systemBold><colour red=204 green=48 blue=0 alpha=255>-5</colour></font>" &&
          runtime.text.at(0x2005u).empty() &&
          runtime.text.at(0x2006u) ==
              "<font name=systemBold><colour red=20 green=84 blue=208 alpha=255>+10</colour></font>",
          "five formatted stat differences");
    Check(runtime.colours.size() == 1u &&
          runtime.colours[0].red == 1.0f &&
          runtime.calls.front() == "read" &&
          runtime.calls[1] == "clear-icon-state" &&
          runtime.calls.back() == "cleanup-text" &&
          runtime.calls[runtime.calls.size() - 2] == "cleanup-preview",
          "white icon and cleanup order");
}

void CheckUnavailableAndEmpty() {
    Runtime unavailable;
    unavailable.words[Runtime::item + 0x168u + 4u] = 11u;
    (void)pal4::TradeWindowPlayerSlot::Update(
        Runtime::slot, 1u, Runtime::item, unavailable);
    Check(unavailable.visibility.size() == 12u &&
          std::all_of(unavailable.visibility.end() - 5,
                      unavailable.visibility.end(),
                      [](Visibility value) { return !value.visible; }) &&
          unavailable.colours.size() == 1u &&
          unavailable.colours[0].red == 0.57647061f &&
          unavailable.text.size() == 1u &&
          unavailable.text.at(0x2007u) == "10",
          "unavailable item hides stats and dims icon");

    Runtime empty;
    (void)pal4::TradeWindowPlayerSlot::Update(
        Runtime::slot, 1u, 0u, empty);
    Check(empty.visibility.size() == 7u && empty.colours.empty() &&
          empty.text.size() == 6u &&
          empty.text.at(0x2002u).empty() &&
          empty.text.at(0x2006u).empty(),
          "empty item clears five stat texts");

    Runtime inactive;
    inactive.words[Runtime::player + 0xb08u] = 0u;
    (void)pal4::TradeWindowPlayerSlot::Update(
        Runtime::slot, 1u, Runtime::item, inactive);
    Check(inactive.visibility.size() == 7u &&
          std::all_of(inactive.visibility.begin(), inactive.visibility.end(),
                      [](Visibility value) { return !value.visible; }) &&
          inactive.text.size() == 6u && inactive.colours.empty(),
          "inactive player remains hidden with cleared stat text");
}

void CheckEdgeCasesAndCleanup() {
    Runtime zero_threshold;
    zero_threshold.words[Runtime::item + 0x168u + 4u] = 0u;
    (void)pal4::TradeWindowPlayerSlot::Update(
        Runtime::slot, 1u, Runtime::item, zero_threshold);
    Check(zero_threshold.colours.size() == 1u &&
          zero_threshold.queried_ids.empty(),
          "nonpositive threshold skips preview");

    Runtime failing;
    failing.throw_finish = true;
    bool threw = false;
    try {
        (void)pal4::TradeWindowPlayerSlot::Update(
            Runtime::slot, 1u, Runtime::item, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.calls.back() == "cleanup-preview" &&
          std::count(failing.calls.begin(), failing.calls.end(),
                     "cleanup-preview") == 1,
          "snapshot exception releases preview state");

    Runtime absent;
    absent.found_player = 0u;
    threw = false;
    try {
        (void)pal4::TradeWindowPlayerSlot::Update(
            Runtime::slot, 1u, Runtime::item, absent);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Check(threw && absent.visibility.size() == 7u &&
          absent.calls.back() == "icon-image",
          "direct invalid player call rejects native null dereference");
}

void CheckHide() {
    Runtime runtime;
    pal4::TradeWindowPlayerSlot::Hide(Runtime::slot, runtime);
    constexpr std::array<std::uint32_t, 7> order{
        0x2002u, 0x2003u, 0x2004u, 0x2005u,
        0x2006u, 0x2000u, 0x2001u};
    Check(runtime.visibility.size() == order.size(),
          "hide affects seven windows");
    for (std::size_t index = 0u; index < order.size(); ++index)
        Check(runtime.visibility[index].window == order[index] &&
              !runtime.visibility[index].visible,
              "hide window order and state");
    Check(runtime.text.empty() && runtime.colours.empty() &&
          std::count(runtime.calls.begin(), runtime.calls.end(),
                     "player") == 0,
          "hide does not query player or edit other controls");
}

}  // namespace

int main() {
    try {
        CheckQualified();
        CheckUnavailableAndEmpty();
        CheckEdgeCasesAndCleanup();
        CheckHide();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
