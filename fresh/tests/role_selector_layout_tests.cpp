#include "pal4/role_selector_layout.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

struct Runtime final : pal4::IRoleSelectorLayoutRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::unordered_map<std::uint32_t, float> window_x;
    std::unordered_map<std::uint32_t,
                       std::array<std::uint32_t, 2>> positions;
    std::array<std::uint32_t, 4> players{
        0x2000u, 0x3000u, 0x4000u, 0x5000u};
    std::array<std::int32_t, 4> keys{30, 10, 20, 40};
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<std::uint32_t> navigation;
    std::vector<pal4::RoleSelectorScratch> releases;
    std::vector<std::string> calls;
    std::uint32_t text_calls = 0u;
    std::uint32_t width_calls = 0u;
    std::uint32_t image_calls = 0u;
    float close_x = -999.0f;

    std::uint32_t GetPlayer(std::uint32_t index) override {
        calls.emplace_back("get-player");
        Check(index < 4u, "player index");
        return players[index];
    }
    std::int32_t GetMaximum(std::uint32_t,
                             std::uint32_t stat) override {
        return stat == 0u ? 100 : 80;
    }
    std::int32_t GetValue(std::uint32_t,
                           std::uint32_t stat) override {
        return stat == 7u ? 50 : stat == 9u ? 40 : 75;
    }
    void SetText(std::uint32_t,
                 const std::string&) override {
        ++text_calls;
    }
    void SetWidth(std::uint32_t, float) override {
        ++width_calls;
    }
    void SetImage(std::uint32_t, const char*,
                  const char*) override {
        ++image_calls;
    }
    std::uint32_t CleanupTextBuffer() override {
        return 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    float ReadFloat(std::uint32_t address) override {
        return floats.at(address);
    }
    std::int32_t GetSortKey(std::uint32_t player) override {
        for (std::size_t index = 0u; index < players.size(); ++index)
            if (players[index] == player) return keys[index];
        throw std::runtime_error("unknown sort player");
    }
    void SetVisible(std::uint32_t window,
                    bool visible) override {
        calls.emplace_back(visible ? "show" : "hide");
        visibility.emplace_back(window, visible);
    }
    void AppendNavigation(std::uint32_t navigation_object,
                          std::uint32_t button) override {
        calls.emplace_back("append");
        Check(navigation_object == 0x102cu,
              "navigation array object");
        navigation.push_back(button);
        words[0x1034u] += 4u;
    }
    void SetPosition(
        std::uint32_t window,
        const std::array<std::uint32_t, 2>& position) override {
        calls.emplace_back("position");
        positions[window] = position;
        window_x[window] = std::bit_cast<float>(position[0]);
    }
    float GetX(std::uint32_t window) override {
        calls.emplace_back("get-x");
        return window_x.at(window);
    }
    float GetWidth(std::uint32_t window) override {
        calls.emplace_back("get-width");
        return window == 0x9000u ? 40.0f : 50.0f;
    }
    void SetX(std::uint32_t window, float x) override {
        calls.emplace_back("set-x");
        Check(window == 0x9000u, "close panel x target");
        close_x = x;
    }
    std::uint32_t ReleaseScratch(
        pal4::RoleSelectorScratch scratch) override {
        calls.emplace_back("release");
        releases.push_back(scratch);
        return scratch == pal4::RoleSelectorScratch::Players
            ? 0x1111u : 0xabcdu;
    }
};

void Seed(Runtime& runtime,
          const std::array<bool, 4>& active) {
    runtime.words[0x1030u] = 0x6000u;
    runtime.words[0x1034u] = 0x6008u;
    runtime.words[0x1158u] = 0x9000u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto child = 0x1048u + index * 52u;
        runtime.words[child] = 0x7000u + index * 0x100u;
        for (std::uint32_t field = 1u; field <= 8u; ++field)
            runtime.words[child + field * 4u] =
                0xa000u + index * 0x1000u + field * 0x100u;
        runtime.words[child + 28u] = 0x8000u + index * 0x100u;
        runtime.floats[child + 36u] = 100.0f;
        runtime.floats[child + 40u] = 80.0f;
        runtime.floats[child + 44u] = 60.0f;
        runtime.words[runtime.players[index] + 0xb08u] =
            active[index] ? 1u : 0u;
        runtime.words[runtime.players[index] + 2632u] = index;
        const float panel_x =
            (std::array<float, 4>{10.0f, 100.0f,
                                  200.0f, 300.0f})[index];
        runtime.words[0x1118u + index * 8u] =
            std::bit_cast<std::uint32_t>(panel_x);
        runtime.words[0x111cu + index * 8u] =
            std::bit_cast<std::uint32_t>(1.0f + index);
        runtime.words[0x1138u + index * 8u] =
            std::bit_cast<std::uint32_t>(panel_x + 5.0f);
        runtime.words[0x113cu + index * 8u] =
            std::bit_cast<std::uint32_t>(2.0f + index);
    }
}

void CheckLayout(const std::array<bool, 4>& active,
                 const std::vector<std::uint32_t>& expected_roles,
                 float expected_close_x) {
    Runtime runtime;
    Seed(runtime, active);
    Check(pal4::RoleSelectorLayout::Layout(
              0x1000u, runtime) == 0xabcdu &&
          runtime.words.at(0x1034u) ==
              0x6000u + expected_roles.size() * 4u &&
          runtime.words.at(0x103cu) == 0u,
          "navigation reset, append and disabled count");
    Check(runtime.visibility.size() ==
              8u + expected_roles.size() * 2u,
          "four panel/button pairs hidden then selected shown");
    for (std::size_t index = 0u; index < 8u; ++index)
        Check(!runtime.visibility[index].second,
              "all controls hidden first");
    Check(runtime.navigation.size() == expected_roles.size() &&
          runtime.releases ==
              (std::vector<pal4::RoleSelectorScratch>{
                  pal4::RoleSelectorScratch::Players,
                  pal4::RoleSelectorScratch::Displayed}) &&
          runtime.text_calls == expected_roles.size() * 3u &&
          runtime.width_calls == expected_roles.size() * 3u &&
          runtime.image_calls == expected_roles.size(),
          "sorted roles, composed stat updates and scratch cleanup");
    const auto first_slot = expected_roles.size() <= 2u ? 1u : 0u;
    for (std::size_t index = 0u; index < expected_roles.size(); ++index) {
        const auto child = 0x1048u +
            static_cast<std::uint32_t>(index) * 52u;
        const auto panel = 0x7000u +
            static_cast<std::uint32_t>(index) * 0x100u;
        const auto button = 0x8000u +
            static_cast<std::uint32_t>(index) * 0x100u;
        const auto slot = first_slot + index;
        Check(runtime.words.at(child + 48u) ==
                  expected_roles[index] &&
              runtime.navigation[index] == button &&
              runtime.positions.at(panel)[0] ==
                  runtime.words.at(0x1118u +
                                   static_cast<std::uint32_t>(slot) * 8u) &&
              runtime.positions.at(button)[0] ==
                  runtime.words.at(0x1138u +
                                   static_cast<std::uint32_t>(slot) * 8u),
              "sort order and centered position slot");
    }
    if (expected_roles.empty()) {
        Check(runtime.close_x == -999.0f &&
              runtime.positions.empty(),
              "empty player list leaves close panel untouched");
    } else {
        Check(Near(runtime.close_x, expected_close_x),
              "close panel centered over visible role span");
    }
}

void CheckStableTie() {
    Runtime runtime;
    Seed(runtime, {true, true, true, false});
    runtime.keys[1] = 20;
    runtime.keys[2] = 20;
    (void)pal4::RoleSelectorLayout::Layout(0x1000u, runtime);
    Check(runtime.words.at(0x1078u) == 1u &&
          runtime.words.at(0x10acu) == 2u,
          "equal sort keys preserve source order");
}

}  // namespace

int main() {
    try {
        CheckLayout({false, false, false, false}, {}, -999.0f);
        CheckLayout({false, false, true, false}, {2u}, 105.0f);
        CheckLayout({true, false, true, false},
                    {2u, 0u}, 155.0f);
        CheckLayout({true, true, true, false},
                    {1u, 2u, 0u}, 110.0f);
        CheckLayout({true, true, true, true},
                    {1u, 2u, 0u, 3u}, 160.0f);
        CheckStableTie();
        std::cout << "role_selector_layout_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
