#include "pal4/magic_window_stunt_image_updater.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct ImageCall {
    std::uint32_t window{};
    std::string imageset;
    std::string image;
};

struct Runtime final : pal4::MagicWindowStuntImageRuntimeStub {
    std::vector<std::string> calls;
    std::vector<ImageCall> images;
    std::vector<std::pair<std::uint32_t, bool>> visible;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> button_keys;
    std::vector<std::uint32_t> lookup_keys;
    std::vector<std::uint32_t> keys;
    std::map<std::uint32_t, pal4::MagicDbStuntRecord> records;
    std::uint32_t player{0x9000u};
    std::int32_t selected_index{};

    void SetText(std::uint32_t window, std::string_view text) override {
        calls.push_back("text:" + std::to_string(window) + ":" + std::string(text));
    }
    void SetImage(std::uint32_t window, std::string_view set,
                  std::string_view image) override {
        images.push_back({window, std::string(set), std::string(image)});
        calls.push_back("image:" + std::string(image));
    }
    void SetVisible(std::uint32_t window, bool shown) override {
        visible.emplace_back(window, shown);
        calls.push_back(std::string(shown ? "show:" : "hide:") + std::to_string(window));
    }
    void SetStuntPanelVisible(std::uint32_t object, bool shown) override {
        Check(object == 0x3000u && shown, "show stunt panel");
        calls.push_back("stunt-panel");
    }
    std::uint32_t GetPlayer(std::int32_t index) override {
        selected_index = index;
        calls.push_back("player");
        return player;
    }
    void SetMagicPanelVisible(std::uint32_t object, bool shown) override {
        Check(object == 0x3000u && !shown, "hide magic panel");
        calls.push_back("magic-panel");
    }
    std::vector<std::uint32_t> StuntKeys(std::uint32_t selected_player) override {
        Check(selected_player == player, "player collection owner");
        calls.push_back("iterator");
        return keys;
    }
    std::optional<pal4::MagicDbStuntRecord> FindStunt(std::uint32_t key) override {
        lookup_keys.push_back(key);
        calls.push_back("lookup:" + std::to_string(key));
        const auto found = records.find(key);
        if (found == records.end()) return std::nullopt;
        return found->second;
    }
    std::string ReadNameUtf8(std::uint32_t address) override {
        calls.push_back("name:" + std::to_string(address));
        return address == 0 ? "" : "stunt" + std::to_string(address);
    }
    void SetButtonKey(std::uint32_t button, std::uint32_t key) override {
        button_keys.emplace_back(button, key);
        calls.push_back("key:" + std::to_string(key));
    }
    void UpdateStuntDetails(std::uint32_t object) override {
        Check(object == 0x3000u, "details owner");
        calls.push_back("details");
    }
};

pal4::MagicWindowPanelState Panel() {
    pal4::MagicWindowPanelState panel;
    for (std::uint32_t offset = 0; offset < 0x170u; offset += 4u)
        panel.At(offset) = 0x1000u + offset;
    return panel;
}

pal4::MagicWindowState State() {
    pal4::MagicWindowState state;
    state.selected_character_index = 3;
    return state;
}

void CheckCommon(const Runtime& runtime) {
    constexpr std::array<const char*, 5> small{
        "5water_s", "5fire_s", "5thurder_s", "5wind_s", "5earth_s"};
    Check(runtime.calls[0] == "text:4232:" && runtime.calls[1] == "text:4236:",
          "clear two info fields first");
    Check(runtime.images.size() >= 6u, "five elements and stunt image");
    for (std::uint32_t i = 0; i < 5u; ++i)
        Check(runtime.images[i].window == 0x1140u + 4u * i &&
                  runtime.images[i].imageset == "maimai0" &&
                  runtime.images[i].image == small[i], "small element image order");
    Check(runtime.images[5].window == 0x1154u &&
              runtime.images[5].imageset == "zi0" &&
              runtime.images[5].image == "teji", "stunt image");
    constexpr std::array<std::uint32_t, 6> labels{
        0x94u, 0x90u, 0x98u, 0xa0u, 0x9cu, 0xa4u};
    for (std::uint32_t i = 0; i < labels.size(); ++i)
        Check(runtime.visible[i] == std::pair<std::uint32_t, bool>{0x1000u + labels[i], false},
              "six counters hidden before player lookup");
    Check(runtime.selected_index == 3, "selected player index");
}

void TestMissingPlayer() {
    Runtime runtime;
    runtime.player = 0;
    Check(pal4::MagicWindowStuntImageUpdater::Update(
              0x3000u, State(), Panel(), runtime) == 0u,
          "missing player return");
    CheckCommon(runtime);
    Check(runtime.calls.back() == "player" && runtime.visible.size() == 6u &&
              runtime.lookup_keys.empty(), "absent player stops after common setup");
}

void TestEmptyCollection() {
    Runtime runtime;
    Check(pal4::MagicWindowStuntImageUpdater::Update(
              0x3000u, State(), Panel(), runtime) == 0u,
          "empty player collection");
    CheckCommon(runtime);
    Check(runtime.visible.size() == 13u, "seven stunt buttons hidden");
    for (std::uint32_t i = 0; i < 7u; ++i)
        Check(runtime.visible[6u + i] ==
                  std::pair<std::uint32_t, bool>{0x1108u + 4u * i, false},
              "stunt row hide order");
    Check(runtime.calls[runtime.calls.size() - 2] == "iterator" &&
              runtime.calls.back() == "details", "details updated for empty collection");
}

void TestSparseCollectionAndLimit() {
    Runtime runtime;
    runtime.keys = {5u, 1u, 2u, 3u, 4u, 6u, 7u, 8u, 9u, 10u};
    for (const auto key : runtime.keys) {
        if (key == 1u || key == 4u) continue;
        runtime.records.emplace(key, pal4::MagicDbStuntRecord{
            key, key == 5u ? 0u : key == 2u ? 2u : 9u, 0u, 200u + key});
    }
    Check(pal4::MagicWindowStuntImageUpdater::Update(
              0x3000u, State(), Panel(), runtime) == 7u,
          "seven valid records displayed");
    CheckCommon(runtime);
    Check(runtime.lookup_keys ==
              std::vector<std::uint32_t>{5u, 1u, 2u, 3u, 4u, 6u, 7u, 8u, 9u},
          "invalid records skipped and iterator stops after seven visible rows");
    Check(runtime.button_keys ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x1108u, 5u}, {0x110cu, 2u}, {0x1110u, 3u},
                  {0x1114u, 6u}, {0x1118u, 7u}, {0x111cu, 8u},
                  {0x1120u, 9u}}), "button keys and compacted row order");
    Check(runtime.images.size() == 13u &&
              runtime.images[6].window == 0x1124u &&
              runtime.images[6].imageset == "gy0" &&
              runtime.images[6].image == "skillattack" &&
              runtime.images[7].image == "skillcure" &&
              runtime.images[8].image == "skillbuff",
          "icon offset and kind mapping");
    Check(runtime.visible.size() == 20u && runtime.calls.back() == "details",
          "seven rows shown and details refreshed last");
    const auto key_call = std::find(runtime.calls.begin(), runtime.calls.end(), "key:5");
    const auto show_call = std::find(runtime.calls.begin(), runtime.calls.end(), "show:4360");
    Check(key_call < show_call, "record key written before row is shown");
}

}  // namespace

int main() {
    TestMissingPlayer();
    TestEmptyCollection();
    TestSparseCollectionAndLimit();
}
