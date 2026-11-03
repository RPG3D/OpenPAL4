#include "pal4/system_setting_stats.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingStatsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::array<std::int32_t, 7> entries{
        2, 3, 128, 256, 100000, std::numeric_limits<std::int32_t>::max(),
        std::numeric_limits<std::int32_t>::min()};
    std::vector<std::tuple<std::uint32_t, std::uint32_t, double>> stats;
    std::vector<std::tuple<std::uint32_t, std::uint32_t, std::int32_t>> integers;
    std::vector<std::pair<std::uint32_t, bool>> selected;
    std::vector<std::pair<std::uint32_t, float>> sliders;
    std::vector<std::uint32_t> entry_indices;
    std::uint32_t inventory_calls = 0u;
    std::uint32_t global_calls = 0u;
    std::uint32_t game_calls = 0u;
    std::uint32_t audio_calls = 0u;
    std::uint32_t end_calls = 0u;
    bool fail_format = false;
    float used = 3.0f;
    std::int32_t available = 12;
    double ratio_one = 0.125;

    Runtime() {
        for (std::uint32_t i = 0; i < 12u; ++i) {
            words[0x4000u + 440u + 4u * i] = 0x1000u + i;
        }
        for (std::uint32_t i = 0; i < 10u; ++i) {
            words[0x4000u + 200u + 4u * i] = 0x2000u + i;
        }
        words[0x4000u + 496u] = 0x3000u;
        words[0x4000u + 500u] = 0x3001u;
        words[0x6000u] = 0u;
        words[0x6000u + 4u] = 2u;
        words[0x6000u + 8u] = 0u;
        words[0x6000u + 12u] = 1u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == 0x7000u || address == 0x7004u,
              "audio field offsets");
        return address == 0x7000u ? 0.25f : 0.75f;
    }
    std::uint32_t GlobalData() override {
        ++global_calls;
        return 0x6000u;
    }
    double CalculateStatValue(std::uint32_t data) override {
        Check(data == 0x6000u, "first stat data");
        return 17.5;
    }
    float UsedSaveCount() override { return used; }
    std::int32_t AvailableSaveCount() override { return available; }
    void InitializeInventory() override { ++inventory_calls; }
    double InventoryRatioOne() override { return ratio_one; }
    double InventoryRatioTwo() override { return 0.5f; }
    double NormalizedStat(std::uint32_t data) override {
        Check(data == 0x6000u, "normalized stat data");
        return 0.625f;
    }
    std::uint32_t Game() override { ++game_calls; return 0x8000u; }
    std::int32_t GameEntry(std::uint32_t game,
                           std::uint32_t index) override {
        Check(game == 0x8000u && index < entries.size(), "game entry");
        entry_indices.push_back(index);
        return entries[index];
    }
    void SetFormattedStat(std::uint32_t window,
                          std::uint32_t trigger_id,
                          double value) override {
        if (fail_format) throw std::runtime_error("format");
        stats.emplace_back(window, trigger_id, value);
    }
    void SetFormattedInteger(std::uint32_t window,
                          std::uint32_t trigger_id,
                          std::int32_t value) override {
        integers.emplace_back(window, trigger_id, value);
    }
    void SetSelected(std::uint32_t button, bool value) override {
        selected.emplace_back(button, value);
    }
    std::uint32_t AudioManager() override {
        ++audio_calls;
        return 0x7000u;
    }
    void SetSliderValue(std::uint32_t slider, float value) override {
        sliders.emplace_back(slider, value);
    }
    std::uint32_t EndTextBuffer() override {
        ++end_calls;
        return 0x55u;
    }
};

void CheckStats(const Runtime& runtime) {
    constexpr double expected[] = {17.5, 25.0, 12.5, 50.0, 62.5, 40.0, 60.0};
    Check(runtime.stats.size() == 7u && runtime.integers.size() == 5u,
          "seven numeric and five integer labels");
    for (std::size_t index = 0; index < 7u; ++index) {
        const auto [window, trigger, value] = runtime.stats[index];
        Check(window == 0x1000u + index && trigger == 156u + index &&
              std::abs(value - expected[index]) < 0.001,
              "numeric stat trigger, window and formula");
    }
    for (std::size_t index = 0; index < 5u; ++index) {
        const auto [window, trigger, value] = runtime.integers[index];
        Check(window == 0x1007u + index && trigger == 163u + index &&
              value == runtime.entries[index + 2u],
              "full-width integer stat trigger and value");
    }
    Check(runtime.entry_indices == std::vector<std::uint32_t>{
          0u, 1u, 2u, 3u, 4u, 5u, 6u} &&
          runtime.global_calls == 6u && runtime.game_calls == 7u &&
          runtime.inventory_calls == 2u && runtime.audio_calls == 2u &&
          runtime.end_calls == 1u,
          "subsystem call counts");
}

void CheckSelection(const Runtime& runtime,
                    std::array<std::uint32_t, 4> expected) {
    Check(runtime.selected.size() == 14u,
          "ten reset and four selected calls");
    for (std::size_t index = 0; index < 10u; ++index) {
        Check(runtime.selected[index] ==
              std::pair<std::uint32_t, bool>{
                  0x2000u + static_cast<std::uint32_t>(index), false},
              "reset selection order");
    }
    for (std::size_t index = 0; index < 4u; ++index) {
        Check(runtime.selected[10u + index] ==
              std::pair<std::uint32_t, bool>{expected[index], true},
              "global setting to selected button");
    }
    Check(runtime.sliders == std::vector<std::pair<std::uint32_t, float>>{
          {0x3000u, 0.75f}, {0x3001u, 0.25f}},
          "inverted audio values");
}

}  // namespace

int main() {
    Runtime normal;
    Check(pal4::SystemSettingStats::Refresh(0x4000u, normal) == 0x55u,
          "returns text-buffer cleanup result");
    CheckStats(normal);
    CheckSelection(normal, {0x2002u, 0x2004u, 0x2006u, 0x2009u});

    Runtime precision;
    precision.used = 1.0f;
    precision.available = 3;
    precision.ratio_one = 1.0 / 3.0;
    (void)pal4::SystemSettingStats::Refresh(0x4000u, precision);
    const auto third_percent = static_cast<float>(100.0 / 3.0);
    Check(std::get<2>(precision.stats[1]) == third_percent &&
              std::get<2>(precision.stats[2]) == third_percent,
          "percentages round to float only after division and multiplication");

    Runtime large_count;
    large_count.used = 16777216.0f;
    large_count.available = 16777217;
    (void)pal4::SystemSettingStats::Refresh(0x4000u, large_count);
    Check(std::get<2>(large_count.stats[1]) ==
              static_cast<float>(16777216.0 / 16777217.0 * 100.0) &&
              std::get<2>(large_count.stats[1]) != 100.0,
          "available count retains integer precision beyond float exact range");

    Runtime no_records;
    no_records.available = 0;
    (void)pal4::SystemSettingStats::Refresh(0x4000u, no_records);
    Check(std::isinf(std::get<2>(no_records.stats[1])),
          "positive over zero preserves original floating infinity");
    no_records.used = 0.0f;
    (void)pal4::SystemSettingStats::Refresh(0x4000u, no_records);
    Check(std::isnan(std::get<2>(no_records.stats[8])),
          "zero over zero preserves original floating NaN");

    Runtime alternate;
    alternate.words[0x6004u] = 99u;
    alternate.words[0x600cu] = 0u;
    alternate.words[0x6000u] = 2u;
    alternate.words[0x6008u] = 1u;
    alternate.entries[0] = 0;
    alternate.entries[1] = 0;
    (void)pal4::SystemSettingStats::Refresh(0x4000u, alternate);
    Check(alternate.selected.size() == 13u &&
          alternate.selected[10] == std::pair{0x2003u, true} &&
          alternate.selected[11] == std::pair{0x2007u, true} &&
          alternate.selected[12] == std::pair{0x2008u, true} &&
          std::get<2>(alternate.stats[5]) == 0.0 &&
          std::get<2>(alternate.stats[6]) == 0.0,
          "invalid difficulty, other options and zero ratio denominator");

    Runtime failure;
    failure.fail_format = true;
    bool caught = false;
    try {
        (void)pal4::SystemSettingStats::Refresh(0x4000u, failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failure.end_calls == 1u,
          "text buffer released after format exception");
}
