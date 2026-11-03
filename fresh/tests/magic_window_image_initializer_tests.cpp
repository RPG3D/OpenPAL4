#include "pal4/magic_window_image_initializer.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MagicWindowImageSetupRuntimeStub,
                       pal4::IMagicWindowSkillRowsRuntime,
                       pal4::MagicWindowNumericRuntimeStub {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> keys;
    std::vector<std::uint32_t> triggers;
    std::vector<std::string> button_images;
    std::vector<std::string> icons;
    std::vector<std::uint32_t> total_keys;
    std::vector<std::pair<std::uint32_t, bool>> enabled;
    std::uint32_t player{0x8000u};
    std::int32_t current{2};
    std::int32_t total{8};
    std::int32_t remaining{5};
    bool state_is_three{};
    std::uint32_t total_key{};
    std::uint32_t role{};
    std::optional<std::int32_t> formatted;

    void SetText(std::uint32_t window, std::string_view value) override {
        calls.push_back("text:" + std::to_string(window) + ":" + std::string(value));
    }
    void SetImage(std::uint32_t, std::string_view, std::string_view) override {
        calls.push_back("setup-image");
    }
    void SetVisible(std::uint32_t window, bool value) override {
        Check(value, "row and label visibility");
        calls.push_back("visible:" + std::to_string(window));
    }
    void SetStuntPanelVisible(std::uint32_t, bool) override { calls.push_back("stunt"); }
    std::uint32_t GetPlayer(std::int32_t) override {
        calls.push_back("player");
        return player;
    }
    void SetMagicPanelVisible(std::uint32_t, bool) override { calls.push_back("magic"); }
    void ResetStuntPanel(std::uint32_t) override { calls.push_back("reset"); }
    std::string ReadNameUtf8(std::uint32_t address) override {
        calls.push_back("name:" + std::to_string(address));
        return "skill" + std::to_string(address);
    }
    std::int32_t CurrentValue(std::uint32_t selected_player,
                              const pal4::MagicDbSkillRecord&) override {
        Check(selected_player == player, "row player");
        calls.push_back("current");
        return current;
    }
    bool SkillStateIsThree(const pal4::MagicDbSkillRecord&) override {
        calls.push_back("state");
        return state_is_three;
    }
    std::int32_t RemainingValue(std::uint32_t selected_player) override {
        Check(selected_player == player, "action remaining player");
        calls.push_back("action-remaining");
        return remaining;
    }
    std::string ActionText(std::uint32_t trigger,
                           std::optional<std::int32_t> value) override {
        calls.push_back("action-text");
        triggers.push_back(trigger);
        formatted = value;
        return "action";
    }
    void SetIcon(std::uint32_t window, std::string_view set,
                 std::string_view image) override {
        Check(set == "gy0", "icon imageset");
        calls.push_back("icon:" + std::to_string(window));
        icons.emplace_back(image);
    }
    void SetButtonImages(std::uint32_t window,
                         const pal4::MagicSkillButtonImages& images) override {
        calls.push_back("button-images:" + std::to_string(window));
        button_images.emplace_back(images.normal);
        Check(!images.highlight.empty() && !images.disabled.empty() &&
                  !images.pushed.empty(), "all button states supplied");
    }
    void SetEnabled(std::uint32_t window, bool value) override {
        calls.push_back("enabled");
        enabled.emplace_back(window, value);
    }
    void SetButtonRecordKey(std::uint32_t window, std::uint32_t key) override {
        calls.push_back("key:" + std::to_string(window));
        keys.push_back(key);
    }
    void UpdateDetails(std::uint32_t object) override {
        Check(object == 0x3000u, "detail object");
        calls.push_back("details");
    }
    std::uint32_t RoleId(std::uint32_t selected_player) override {
        Check(selected_player == player, "role ID player");
        calls.push_back("role-id");
        return 13u;
    }
    std::int32_t QueryPlayerValue(std::uint32_t selected_player,
                                  std::uint32_t selected_role) override {
        Check(selected_player == player, "numeric player");
        role = selected_role;
        calls.push_back("numeric-base");
        return 11;
    }
    std::int32_t RemainingValue(std::uint32_t selected_player,
                                 std::int32_t base) override {
        Check(selected_player == player && base == 11, "numeric remaining");
        calls.push_back("numeric-remaining");
        return -3;
    }
    std::int32_t TotalValue(std::uint32_t selected_player,
                            std::uint32_t key) override {
        Check(selected_player == player, "total player");
        total_key = key;
        total_keys.push_back(key);
        calls.push_back("total");
        return total;
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
    state.slot_order = {5u, 2u, 1u, 0u, 3u, 4u};
    state.category_position = 1u;
    return state;
}

std::size_t Initialize(Runtime& runtime,
                       std::span<const pal4::MagicDbSkillRecord> records,
                       const pal4::MagicWindowState& state = State()) {
    const auto panel = Panel();
    return pal4::MagicWindowImageInitializer::Initialize(
        0x3000u, state, panel, records,
        runtime, runtime, runtime);
}

void TestSelectedCategory() {
    Runtime runtime;
    auto state = State();
    state.category_position = 2u;
    const std::array<pal4::MagicDbSkillRecord, 2> records{{
        {.key = 10u, .category = 2u},
        {.key = 11u, .category = 1u,
         .threshold_3c = 5u, .minimum_total_40 = 8u}}};
    Check(Initialize(runtime, records, state) == 1u &&
              runtime.keys == std::vector<std::uint32_t>{11u} &&
              runtime.total_keys == std::vector<std::uint32_t>{1u, 1u},
          "selected slot controls filtering, row total and final total");
}

void TestMissingPlayer() {
    Runtime runtime;
    runtime.player = 0;
    const std::array<pal4::MagicDbSkillRecord, 1> records{{{.category = 2u}}};
    Check(Initialize(runtime, records) == 0u, "missing player return");
    Check(runtime.calls.back() == "player" && runtime.triggers.empty() &&
              runtime.keys.empty(), "missing player stops before catalog and numeric display");
}

void TestEmptyCatalog() {
    Runtime runtime;
    const std::array<pal4::MagicDbSkillRecord, 1> records{{{.category = 8u}}};
    Check(Initialize(runtime, records) == 0u, "unmatched catalog");
    Check(runtime.keys.empty() && runtime.triggers.empty() &&
              runtime.calls.back() == "text:4248:8" &&
              runtime.role == 13u && runtime.total_key == 2u,
          "no rows skips details but updates three numeric fields");
}

void TestRowsAndActions() {
    std::array<pal4::MagicDbSkillRecord, 9> records{};
    for (std::size_t i = 0; i < records.size(); ++i) {
        records[i].key = 100u + static_cast<std::uint32_t>(i);
        records[i].category = i == 1u ? 8u : 2u;
        records[i].name_address = 200u + static_cast<std::uint32_t>(i);
        records[i].kind = i % 3u == 0u ? 0u : 2u;
        records[i].threshold_3c = 5u;
        records[i].minimum_total_40 = 8u;
    }
    Runtime runtime;
    Check(Initialize(runtime, records) == 6u, "six visible rows");
    Check(runtime.keys == std::vector<std::uint32_t>{100u, 102u, 103u, 104u, 105u, 106u},
          "filtered records retain table order and stop after six");
    Check(runtime.triggers.size() == 6u && runtime.triggers.front() == 0xbcu &&
              runtime.formatted == 5 && runtime.enabled.front() ==
                  std::pair<std::uint32_t, bool>{0x10f0u, true},
          "unlearned action text and resource gates");
    Check(runtime.button_images.front() == "bottonskilllearn" &&
              runtime.icons.front() == "skillattack" &&
              runtime.icons[1] == "skillcure", "row button and icon resources");
    const auto details = std::find(runtime.calls.begin(), runtime.calls.end(), "details");
    const auto role_id = std::find(runtime.calls.begin(), runtime.calls.end(), "role-id");
    const auto numeric = std::find(runtime.calls.begin(), runtime.calls.end(), "numeric-base");
    Check(details != runtime.calls.end() && role_id != runtime.calls.end() &&
              numeric != runtime.calls.end() && details < role_id &&
              role_id < numeric && runtime.calls.back() == "text:4248:8",
          "detail refresh precedes final counters");
    Check(runtime.total_keys == std::vector<std::uint32_t>(7u, 2u),
          "row and final totals share selected category key");
    Check(runtime.calls[0] == "text:4232:" && runtime.calls[1] == "text:4236:",
          "common setup precedes row binding");

    runtime = Runtime{};
    runtime.current = 5;
    runtime.state_is_three = false;
    const std::array<pal4::MagicDbSkillRecord, 1> learned{{records[0]}};
    Check(Initialize(runtime, learned) == 1u &&
              runtime.triggers == std::vector<std::uint32_t>{0x107u} &&
              !runtime.enabled.front().second &&
              runtime.button_images.front() == "bottonskill" &&
              std::count(runtime.calls.begin(), runtime.calls.end(), "total") == 1,
          "learned blocked branch omits resource queries");
    runtime = Runtime{};
    runtime.current = 5;
    runtime.state_is_three = true;
    Check(Initialize(runtime, learned) == 1u &&
              runtime.triggers == std::vector<std::uint32_t>{0x16u} &&
              runtime.enabled.front().second, "learned usable branch");
}

}  // namespace

int main() {
    TestMissingPlayer();
    TestEmptyCatalog();
    TestSelectedCategory();
    TestRowsAndActions();
}
