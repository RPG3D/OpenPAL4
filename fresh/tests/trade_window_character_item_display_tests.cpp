#include "pal4/trade_window_character_item_display.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITradeWindowCharacterItemDisplayRuntime {
    static constexpr std::uint32_t frame = 0x1000u;
    static constexpr std::uint32_t item = 0x2000u;
    static constexpr std::uint32_t record = 0x3000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {frame + 0x2cu, 0x4000u}, {frame + 0x30u, 0x4100u},
        {frame + 0x34u, 0x4200u}, {item, record},
        {record + 0x24u, 0x5000u}, {record + 0x34u, 0x5100u},
        {record + 0x54u, 0x5200u}, {record + 0x64u, 0x5300u},
        {0x9134u, static_cast<std::uint32_t>(-41)}};
    std::map<std::uint32_t, std::string> texts{
        {0x5000u, "Name"}, {0x5100u, "Description"},
        {0x5200u, "Object"}, {0x5300u, "Skin"}};
    std::vector<std::pair<std::uint32_t, std::string>> shown;
    std::vector<std::string> calls;
    std::string model_name;
    std::string model_texture;
    std::string model_path;
    std::uint32_t model_frame = 0u;
    unsigned cleanups = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read-word");
        return words.at(address);
    }
    std::uint32_t InventoryBase() override {
        calls.emplace_back("inventory");
        return 0x9000u;
    }
    std::string ReadText(const std::uint32_t address) override {
        calls.emplace_back("read-text");
        return address == 0u ? "" : texts.at(address);
    }
    std::string ConvertToUtf8(const std::string_view text) override {
        calls.emplace_back("utf8");
        return std::string(text);
    }
    void SetWindowText(const std::uint32_t window,
                       const std::string_view text) override {
        shown.emplace_back(window, std::string(text));
        calls.emplace_back("set-text");
    }
    void SetGameObjectDisplay(const std::uint32_t owner,
                              const std::string_view name,
                              const std::string_view texture,
                              const std::string_view path) override {
        model_frame = owner;
        model_name = name;
        model_texture = texture;
        model_path = path;
        calls.emplace_back("model");
    }
    std::uint32_t CleanupTextBuffer() override {
        ++cleanups;
        calls.emplace_back("cleanup");
        return 0x55u;
    }
};

void CheckEmptyItem() {
    Runtime runtime;
    Check(pal4::TradeWindowCharacterItemDisplay::Update(
              Runtime::frame, 0u, runtime) == 0x55u,
          "empty item cleanup return");
    Check(runtime.shown ==
              (std::vector<std::pair<std::uint32_t, std::string>>{
                  {0x4000u, ""}, {0x4200u, "-41"}, {0x4100u, ""}}),
          "empty item text and native update order");
    Check(runtime.model_frame == Runtime::frame &&
              runtime.model_name.empty() &&
              runtime.model_texture.empty() &&
              runtime.model_path.empty() && runtime.cleanups == 1u &&
              runtime.calls[runtime.calls.size() - 2u] == "model" &&
              runtime.calls.back() == "cleanup",
          "empty item clears preview and cleans text buffer");
}

void CheckPopulatedItem() {
    Runtime runtime;
    Check(pal4::TradeWindowCharacterItemDisplay::Update(
              Runtime::frame, Runtime::item, runtime) == 0x55u &&
              runtime.shown ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x4000u, "Name"}, {0x4100u, "Description"},
                      {0x4200u, "-41"}}),
          "item uses inner record fields and signed balance");
    Check(runtime.model_frame == Runtime::frame &&
              runtime.model_name == "Object" &&
              runtime.model_texture == "Skin" &&
              runtime.model_path == "PALObject\\" &&
              runtime.cleanups == 1u,
          "item selects object model and cleans temporary buffer");

    Runtime missing;
    missing.words[Runtime::record + 0x24u] = 0u;
    missing.words[Runtime::record + 0x34u] = 0u;
    missing.words[Runtime::record + 0x54u] = 0u;
    missing.words[Runtime::record + 0x64u] = 0u;
    (void)pal4::TradeWindowCharacterItemDisplay::Update(
        Runtime::frame, Runtime::item, missing);
    Check(missing.shown[0].second.empty() &&
              missing.shown[1].second.empty() &&
              missing.model_name.empty() &&
              missing.model_texture.empty() &&
              missing.model_path == "PALObject\\",
          "null native strings are read as empty strings");
}

void CheckDirectRecordEntry() {
    Runtime runtime;
    runtime.words.erase(Runtime::item);
    Check(pal4::TradeWindowCharacterItemDisplay::UpdateRecord(
              Runtime::frame, Runtime::record, runtime) == 0x55u &&
              runtime.shown ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x4000u, "Name"}, {0x4200u, "-41"},
                      {0x4100u, "Description"}}) &&
              runtime.model_name == "Object" &&
              runtime.model_texture == "Skin",
          "direct record path sets balance before description");

    Runtime empty;
    Check(pal4::TradeWindowCharacterItemDisplay::UpdateRecord(
              Runtime::frame, 0u, empty) == 0x55u &&
              empty.shown ==
                  (std::vector<std::pair<std::uint32_t, std::string>>{
                      {0x4000u, ""}, {0x4200u, "-41"},
                      {0x4100u, ""}}) &&
              empty.model_path.empty() && empty.cleanups == 1u,
          "direct record null path clears preview and detail");
}

}  // namespace

int main() {
    try {
        CheckEmptyItem();
        CheckPopulatedItem();
        CheckDirectRecordEntry();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
