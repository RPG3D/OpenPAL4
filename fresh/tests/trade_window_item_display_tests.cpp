#include "pal4/trade_window_item_display.h"
#include "pal4/trade_window_item_display_slot_runtime.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct ModelCall {
    std::uint32_t frame;
    std::optional<std::string> name;
    std::uint32_t texture;
    std::optional<std::string> path;
};

struct SlotCall {
    std::uint32_t address;
    std::uint32_t player;
    std::uint32_t item;
};

struct Runtime final : pal4::ITradeWindowItemDisplayRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t item = 0x4000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {frame + 0x2cu, 0x2000u}, {frame + 0x30u, 0x2004u},
        {frame + 0x34u, 0x2008u}, {frame + 0x38u, 0x200cu},
        {0x9134u, 1234u}, {item + 0xacu, 0x5000u},
        {item + 0xbcu, 0x5004u}, {item + 0x94u, 0u},
        {item + 0x178u, 23u}, {item + 0xdcu, 0x5008u},
        {item + 0xecu, 0x500cu}};
    std::map<std::uint32_t, std::string> strings{
        {0x5000u, "Sword"}, {0x5004u, "Extra"},
        {0x5008u, "Model"}};
    std::array<std::uint32_t, 4> players{};
    std::map<std::uint32_t, std::string> window_text;
    std::vector<std::string> events;
    std::vector<std::uint32_t> reads;
    std::vector<SlotCall> updated;
    std::vector<std::uint32_t> hidden;
    std::vector<ModelCall> models;
    std::string description = "Description";
    std::uint8_t active = 0u;
    std::uint32_t cleanup_result = 0xdecafu;

    std::uint32_t ReadWord(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == frame + 0xf8u, "active flag address");
        return active;
    }
    std::uint32_t PlayerByIndex(std::uint32_t index) override {
        events.push_back("player:" + std::to_string(index));
        return players.at(index);
    }
    void UpdatePlayerSlot(std::uint32_t slot, std::uint32_t index,
                          std::uint32_t value) override {
        events.push_back("update:" + std::to_string(index));
        updated.push_back({slot, index, value});
    }
    void HidePlayerSlot(std::uint32_t slot) override {
        events.push_back("hide");
        hidden.push_back(slot);
    }
    std::uint32_t InventoryBase() override { return 0x9000u; }
    std::string ReadText(std::uint32_t address) override {
        if (address == 0u) return {};
        return strings.at(address);
    }
    std::string ConvertToUtf8(std::string_view source) override {
        events.push_back("utf8");
        return std::string(source);
    }
    std::string BuildDescription(std::uint32_t value) override {
        Check(value == item, "description item");
        events.push_back("description");
        return description;
    }
    void SetWindowText(std::uint32_t window,
                       std::string_view value) override {
        events.push_back("text:" + std::to_string(window));
        window_text[window] = value;
    }
    void SetGameObjectDisplay(
        std::uint32_t owner, std::optional<std::string_view> name,
        std::uint32_t texture,
        std::optional<std::string_view> path) override {
        events.push_back("model");
        models.push_back({owner,
                          name ? std::optional<std::string>(*name) : std::nullopt,
                          texture,
                          path ? std::optional<std::string>(*path) : std::nullopt});
    }
    std::uint32_t CleanupTextBuffers() override {
        events.push_back("cleanup");
        return cleanup_result;
    }
};

void CheckSlotsAndEmptyItem() {
    Runtime runtime;
    runtime.players = {0x7000u, 0u, 0x7200u, 0x7300u};
    runtime.words[0x7000u + 0xb08u] = 1u;
    runtime.words[0x7200u + 0xb08u] = 0u;
    runtime.words[0x7300u + 0xb08u] = 1u;
    const auto result = pal4::TradeWindowItemDisplay::GetItemDataThunk(
        Runtime::frame, 0u, runtime);
    Check(result == runtime.cleanup_result, "thunk return");
    Check(runtime.updated.size() == 2u &&
          runtime.updated[0].address == Runtime::frame + 0x54u &&
          runtime.updated[0].player == 0u && runtime.updated[0].item == 0u &&
          runtime.updated[1].address == Runtime::frame + 0x74u &&
          runtime.updated[1].player == 3u, "compressed player slots");
    Check(runtime.hidden == std::vector<std::uint32_t>{
              Runtime::frame + 0x94u, Runtime::frame + 0xb4u},
          "remaining slots hidden");
    Check(runtime.window_text == std::map<std::uint32_t, std::string>{
              {0x2000u, ""}, {0x2004u, ""},
              {0x2008u, "1234"}, {0x200cu, ""}},
          "empty item labels");
    Check(runtime.models.size() == 1u &&
          runtime.models[0].frame == Runtime::frame &&
          !runtime.models[0].name && !runtime.models[0].path &&
          runtime.models[0].texture == 0u, "empty item clears model");
    Check(runtime.events == std::vector<std::string>{
              "player:0", "update:0", "player:1", "player:2",
              "player:3", "update:3", "hide", "hide",
              "text:8192", "text:8196", "utf8", "text:8200",
              "text:8204", "model", "cleanup"},
          "empty item call order");
}

void CheckItemTextAndPrice() {
    Runtime runtime;
    const auto result = pal4::TradeWindowItemDisplay::Update(
        Runtime::frame, Runtime::item, runtime);
    Check(result == runtime.cleanup_result, "item return");
    Check(runtime.hidden.size() == 4u && runtime.updated.empty(),
          "no players hide all slots");
    Check(runtime.window_text == std::map<std::uint32_t, std::string>{
              {0x2000u, "Sword"},
              {0x2004u, "Description<br/><colour red=0 green=0 blue=255 alpha=255>(Extra) </colour>"},
              {0x2008u, "1234"}, {0x200cu, "5"}},
          "item name, description and signed quarter price");
    Check(runtime.models.size() == 1u &&
          runtime.models[0].name == "Model" &&
          runtime.models[0].texture == 0x500cu &&
          runtime.models[0].path == "PALWeapon\\", "weapon model");

    Runtime active;
    active.active = 1u;
    active.words[Runtime::item + 0x178u] =
        static_cast<std::uint32_t>(-9);
    active.words[Runtime::item + 0xbcu] = 0u;
    (void)pal4::TradeWindowItemDisplay::Update(
        Runtime::frame, Runtime::item, active);
    Check(active.window_text.at(0x2004u) == "Description" &&
          active.window_text.at(0x200cu) == "-9",
          "active full signed price and absent extra text");
    active.active = 0u;
    (void)pal4::TradeWindowItemDisplay::Update(
        Runtime::frame, Runtime::item, active);
    Check(active.window_text.at(0x200cu) == "-2",
          "inactive negative price rounds toward zero");
}

void CheckCategories() {
    for (std::uint32_t category = 0u; category <= 7u; ++category) {
        Runtime runtime;
        runtime.words[Runtime::item + 0x94u] = category;
        (void)pal4::TradeWindowItemDisplay::Update(
            Runtime::frame, Runtime::item, runtime);
        if (category <= 6u) {
            Check(runtime.models.size() == 1u &&
                  runtime.models[0].path ==
                      (category <= 2u ? "PALWeapon\\" : "PALObject\\"),
                  "category model path");
        } else {
            Check(runtime.models.empty(), "invalid category skips model");
            for (const auto address : runtime.reads)
                Check(address != Runtime::item + 0xdcu &&
                      address != Runtime::item + 0xecu,
                      "invalid category skips model fields");
        }
        Check(runtime.events.back() == "cleanup", "final cleanup");
    }
}

struct SlotRuntime final : pal4::ITradeWindowPlayerSlotRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::map<std::uint32_t, std::string> text;
    std::uint32_t cleanup_count = 0u;

    SlotRuntime() {
        words[0x7000u + 0xb08u] = 1u;
        for (std::uint32_t slot = 0u; slot < 4u; ++slot)
            for (std::uint32_t index = 0u; index < 8u; ++index)
                words[Runtime::frame + 0x54u + slot * 0x20u +
                      index * 4u] = 0x3000u + slot * 8u + index;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x3000u + 0x6b4u && value == 0u,
              "composed slot icon byte");
    }
    std::uint32_t PlayerByIndex(std::uint32_t index) override {
        Check(index == 1u, "composed player index");
        return 0x7000u;
    }
    void SetVisible(std::uint32_t window, bool value) override {
        visibility.emplace_back(window, value);
    }
    std::uint32_t IconImage(std::uint32_t index, std::uint32_t type,
                            std::uint32_t variant) override {
        Check(index == 1u && type == 2u && variant == 0u,
              "composed icon request");
        return 0x8000u;
    }
    void SetIconImage(std::uint32_t window,
                      std::uint32_t image) override {
        Check(window == 0x3000u && image == 0x8000u,
              "composed icon image");
    }
    std::int32_t PlayerStat(std::uint32_t player,
                            std::uint32_t kind) override {
        Check(player == 0x7000u && kind == 4u,
              "composed player stat");
        return 42;
    }
    std::string ConvertToUtf8(std::string_view value) override {
        return std::string(value);
    }
    void SetText(std::uint32_t window,
                 std::string_view value) override {
        text[window] = value;
    }
    void SetIconColour(std::uint32_t,
                       pal4::TradeSlotColour) override {
        throw std::logic_error("empty item must not set colour");
    }
    std::int32_t ClassifySlot(std::uint32_t,
                              std::uint32_t) override {
        throw std::logic_error("empty item must not classify");
    }
    void RefreshEquipment(std::uint32_t) override {
        throw std::logic_error("empty item must not refresh equipment");
    }
    void StartPreview(std::uint32_t) override {
        throw std::logic_error("empty item must not start preview");
    }
    std::uint32_t InventoryBase() override {
        throw std::logic_error("empty item must not query inventory");
    }
    std::uint32_t FindInventoryItem(std::uint32_t, std::uint32_t,
                                    std::uint32_t) override {
        throw std::logic_error("empty item must not search inventory");
    }
    void AddPreviewRecord(std::uint32_t) override {
        throw std::logic_error("empty item must not add preview record");
    }
    std::array<std::int32_t, 5> FinishPreview(
        std::uint32_t) override {
        throw std::logic_error("empty item must not finish preview");
    }
    void CleanupPreview() override {
        throw std::logic_error("empty item has no preview");
    }
    std::uint32_t CleanupTextBuffers() override {
        ++cleanup_count;
        return 0u;
    }
};

void CheckSlotComposition() {
    Runtime display;
    display.players[1] = 0x7000u;
    display.words[0x7000u + 0xb08u] = 1u;
    SlotRuntime slots;
    pal4::TradeWindowItemDisplaySlotRuntime linked(display, slots);
    Check(pal4::TradeWindowItemDisplay::Update(
              Runtime::frame, 0u, linked) == display.cleanup_result,
          "composed display result");
    Check(slots.visibility.size() == 28u &&
          slots.visibility[0] ==
              std::pair<std::uint32_t, bool>{0x3002u, true} &&
          slots.visibility[6] ==
              std::pair<std::uint32_t, bool>{0x3001u, true} &&
          slots.visibility[7] ==
              std::pair<std::uint32_t, bool>{0x300au, false} &&
          slots.visibility.back() ==
              std::pair<std::uint32_t, bool>{0x3019u, false},
          "display composes player update and three hide calls");
    Check(slots.text.size() == 6u &&
          slots.text.at(0x3007u) == "402" &&
          slots.text.at(0x3002u).empty() &&
          slots.cleanup_count == 1u,
          "composed player percentage and empty comparisons");
}

}  // namespace

int main() {
    try {
        CheckSlotsAndEmptyItem();
        CheckItemTextAndPrice();
        CheckCategories();
        CheckSlotComposition();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
