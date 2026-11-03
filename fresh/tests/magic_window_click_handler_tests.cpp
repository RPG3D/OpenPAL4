#include "pal4/magic_window_click_handler.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
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

struct Runtime final : pal4::MagicWindowClickRuntimeStub {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> button_keys;
    std::vector<std::uint32_t> queried_buttons;
    std::vector<std::int32_t> skill_values;
    std::optional<pal4::MagicDbSkillRecord> skill;
    std::uint32_t player{0x9000u};
    std::int32_t remaining{5};
    std::uint32_t stat_base{10u};
    std::uint32_t stat_value{};
    std::uint32_t learned_key{};
    std::uint32_t dialog_key{};
    std::uint32_t callback_address{0xa000u};
    std::uint32_t callback_seen{};
    std::uint32_t callback_deleted{};
    std::uint32_t callback_replacement{};
    bool dialog_visible{};
    bool throw_in_dialog{};
    std::vector<std::pair<std::uint32_t, std::uint32_t>> callback_writes;
    std::int32_t detail_row{-1};
    std::uint32_t category_seen{};
    std::uint32_t amount_seen{};
    std::uint32_t hint_name{};
    std::uint32_t hint_trigger{};
    float hint_x{};
    float hint_y{};
    std::string sound;

    std::uint32_t ButtonKey(std::uint32_t button) override {
        queried_buttons.push_back(button);
        calls.push_back("button-key");
        const auto found = button_keys.find(button);
        return found == button_keys.end() ? 0u : found->second;
    }
    std::optional<pal4::MagicDbSkillRecord> FindSkill(std::uint32_t key) override {
        Check(key == button_keys.at(0x5000u), "skill lookup key");
        calls.push_back("skill-record");
        return skill;
    }
    std::uint32_t GetPlayer(std::int32_t index) override {
        Check(index == 3, "selected character index");
        calls.push_back("player");
        return player;
    }
    std::int32_t SkillValue(std::uint32_t selected_player,
                            std::uint32_t category,
                            std::uint32_t key) override {
        Check(selected_player == player && key == button_keys.at(0x5000u),
              "skill value player and key");
        category_seen = category;
        calls.push_back("skill-value");
        Check(!skill_values.empty(), "configured skill value sequence");
        const auto value = skill_values.front();
        skill_values.erase(skill_values.begin());
        return value;
    }
    std::int32_t RemainingValue(std::uint32_t selected_player) override {
        Check(selected_player == player, "remaining value player");
        calls.push_back("remaining");
        return remaining;
    }
    void AddSkillDelta(std::uint32_t selected_player, std::uint32_t category,
                       std::uint32_t key, std::uint32_t amount) override {
        Check(selected_player == player && key == button_keys.at(0x5000u),
              "delta target");
        category_seen = category;
        amount_seen = amount;
        calls.push_back("add-delta");
    }
    void UpdatePlayerCollection(std::uint32_t selected_player,
                                std::uint32_t category,
                                std::uint32_t amount) override {
        Check(selected_player == player && category == category_seen &&
                  amount == amount_seen, "player collection delta");
        calls.push_back("player-collection");
    }
    std::uint32_t PlayerStatBase(std::uint32_t selected_player,
                                 std::uint32_t category) override {
        Check(selected_player == player && category == category_seen,
              "category stat base");
        calls.push_back("stat-base");
        return stat_base;
    }
    void SetPlayerStat(std::uint32_t selected_player, std::uint32_t category,
                       std::uint32_t value) override {
        Check(selected_player == player && category == category_seen,
              "player stat target");
        stat_value = value;
        calls.push_back("set-stat");
    }
    void InsertLearnedSkill(std::uint32_t selected_player,
                            std::uint32_t category,
                            std::uint32_t key) override {
        Check(selected_player == player && category == category_seen,
              "learned skill collection");
        learned_key = key;
        calls.push_back("learned");
    }
    void ShowLearningHint(std::uint32_t name, std::uint32_t trigger,
                          float x, float y) override {
        hint_name = name;
        hint_trigger = trigger;
        hint_x = x;
        hint_y = y;
        calls.push_back("hint");
    }
    void PlaySound(std::string_view path) override {
        sound = path;
        calls.push_back("sound");
    }
    std::uint32_t AllocateCallback(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "cast callback size");
        calls.push_back("allocate-callback");
        return callback_address;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        callback_writes.emplace_back(address, value);
        calls.push_back("write-callback");
    }
    void ShowCastDialog(bool visible, std::uint32_t& callback,
                        std::uint32_t key) override {
        dialog_visible = visible;
        callback_seen = callback;
        dialog_key = key;
        calls.push_back("dialog");
        if (callback_replacement != 0) callback = callback_replacement;
        if (throw_in_dialog) throw std::runtime_error("dialog failed");
    }
    void DeleteCallback(std::uint32_t callback, std::uint32_t flags) override {
        Check(flags == 1u, "callback delete flags");
        callback_deleted = callback;
        calls.push_back("delete-callback");
    }
    void InitializeImages(std::uint32_t object) override {
        Check(object == 0x3000u, "magic image owner");
        calls.push_back("images");
    }
    void UpdateDetails(std::uint32_t object, std::int32_t row) override {
        Check(object == 0x3000u, "detail owner");
        detail_row = row;
        calls.push_back("details");
    }
};

pal4::MagicWindowPanelState Panel() {
    pal4::MagicWindowPanelState panel;
    for (std::uint32_t offset = 0; offset < 0x170u; offset += 4u)
        panel.At(offset) = 0x1000u + offset;
    return panel;
}

pal4::MagicWindowState State(std::uint32_t category = 2u) {
    pal4::MagicWindowState state;
    state.selected_character_index = 3;
    state.category_position = 1u;
    state.slot_order = {5u, category, 1u, 0u, 3u, 4u};
    return state;
}

Runtime BaseRuntime(std::uint32_t threshold = 5u) {
    Runtime runtime;
    runtime.button_keys[0x5000u] = 44u;
    runtime.button_keys[0x10f0u] = 11u;
    runtime.button_keys[0x10f4u] = 12u;
    runtime.button_keys[0x10f8u] = 44u;
    pal4::MagicDbSkillRecord skill;
    skill.key = 44u;
    skill.name_address = 0x8800u;
    skill.threshold_3c = threshold;
    runtime.skill = skill;
    return runtime;
}

bool Click(const pal4::MagicWindowState& state, Runtime& runtime) {
    return pal4::MagicWindowClickHandler::Handle(
        0x3000u, 0x5000u, state, Panel(), runtime);
}

void TestMissingPlayerAndRecord() {
    auto runtime = BaseRuntime();
    runtime.player = 0;
    Check(Click(State(), runtime) &&
              runtime.calls == std::vector<std::string>{
                  "button-key", "skill-record", "player"},
          "missing player exits after record and player lookup");
    runtime = BaseRuntime();
    runtime.skill.reset();
    Check(Click(State(), runtime) && runtime.calls.size() == 3u,
          "missing record cannot enter a click branch");
}

void TestKnownSkillAndDefaultRow() {
    auto runtime = BaseRuntime();
    runtime.skill_values = {5};
    Check(Click(State(), runtime) && runtime.category_seen == 2u &&
              runtime.detail_row == 2 && runtime.dialog_key == 44u &&
              runtime.dialog_visible && runtime.callback_seen == 0xa000u &&
              runtime.callback_deleted == 0xa000u,
          "known skill selects matching action row and opens cast dialog");
    Check(runtime.callback_writes ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0xa008u, 0x459fb0u}, {0xa00cu, 0u},
                  {0xa010u, 0x3000u}, {0xa000u, 0x843914u}}) &&
              runtime.calls.back() == "delete-callback" &&
              runtime.queried_buttons ==
                  std::vector<std::uint32_t>{0x5000u, 0x10f0u, 0x10f4u, 0x10f8u},
          "cast callback layout, cleanup and row search");

    runtime = BaseRuntime();
    runtime.button_keys[0x10f8u] = 13u;
    runtime.skill_values = {5};
    Check(Click(State(), runtime) && runtime.detail_row == 0 &&
              runtime.queried_buttons.size() == 7u,
          "unmatched button key keeps IDA default row zero");
}

void TestCastCallbackOwnership() {
    auto runtime = BaseRuntime();
    runtime.skill_values = {5};
    runtime.callback_address = 0;
    Check(Click(State(), runtime) && runtime.callback_seen == 0u &&
              runtime.callback_writes.empty() && runtime.callback_deleted == 0u,
          "allocation failure still opens dialog with null callback");

    runtime = BaseRuntime();
    runtime.skill_values = {5};
    runtime.callback_replacement = 0xb000u;
    Check(Click(State(), runtime) && runtime.callback_seen == 0xa000u &&
              runtime.callback_deleted == 0xb000u,
          "dialog-updated callback reference is released");

    runtime = BaseRuntime();
    runtime.skill_values = {5};
    runtime.throw_in_dialog = true;
    bool caught = false;
    try {
        Click(State(), runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.callback_deleted == 0xa000u,
          "callback released when dialog throws");
}

void TestInsufficientRemaining() {
    auto runtime = BaseRuntime();
    runtime.skill_values = {2};
    runtime.remaining = 4;
    Check(Click(State(), runtime) && runtime.calls.back() == "remaining" &&
              runtime.amount_seen == 0u && runtime.detail_row == -1,
          "insufficient signed remaining value has no mutation or refresh");
}

void TestLearningBranches() {
    auto runtime = BaseRuntime();
    runtime.skill_values = {2, 5};
    Check(Click(State(), runtime) && runtime.amount_seen == 5u &&
              runtime.category_seen == 2u && runtime.stat_value == 15u &&
              runtime.learned_key == 44u && runtime.hint_name == 0x8800u &&
              runtime.hint_trigger == 9u && runtime.hint_x == 0.5f &&
              runtime.hint_y == 0.4f &&
              runtime.sound == "gamedata\\palSound\\wa08.wav" &&
              runtime.detail_row == 2, "learning applies delta, stat, hint and sound");
    const auto expected_tail = std::vector<std::string>{
        "add-delta", "player-collection", "stat-base", "set-stat",
        "skill-value", "learned", "hint", "sound", "images", "details"};
    Check(std::vector<std::string>(runtime.calls.end() - expected_tail.size(),
                                   runtime.calls.end()) == expected_tail,
          "learning side effects follow IDA order");

    runtime = BaseRuntime();
    runtime.skill_values = {2, 4};
    Check(Click(State(), runtime) && runtime.learned_key == 0u &&
              runtime.sound.empty() && runtime.calls[runtime.calls.size() - 2] == "images" &&
              runtime.calls.back() == "details",
          "post-update value below threshold still refreshes without learn hint");
}

void TestSignedThresholdAndCategoryFallback() {
    auto runtime = BaseRuntime(0xffffffffu);
    runtime.skill_values = {-2, -1};
    runtime.remaining = -1;
    Check(Click(State(5u), runtime) && runtime.category_seen == 5u &&
              runtime.amount_seen == 0xffffffffu && runtime.stat_value == 4u &&
              runtime.learned_key == 44u,
          "negative threshold uses signed gates and category five as stat base");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(), "stat-base") ==
              runtime.calls.end(), "category five skips indexed base-stat read");
}

}  // namespace

int main() {
    TestMissingPlayerAndRecord();
    TestKnownSkillAndDefaultRow();
    TestCastCallbackOwnership();
    TestInsufficientRemaining();
    TestLearningBranches();
    TestSignedThresholdAndCategoryFallback();
}
