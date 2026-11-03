#include "pal4/minimap_scene_ui_skill_action.h"

#include <cstdlib>
#include <iostream>
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

struct Runtime final : pal4::IMinimapSceneUiSkillRuntime {
    std::vector<std::string> calls;
    std::uint32_t action = 0u;
    std::int32_t resource = 0;
    std::int32_t spent = 0;
    std::uint32_t text = 0xd000u;
    std::uint32_t converted_from = 0u;
    bool acted = false;
    bool secondary_acted = false;
    bool null_manager = false;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return null_manager ? 0u : 0x9000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        if (address == (null_manager ? 0x2c50u : 0x9000u + 0x2c50u))
            return null_manager ? 0u : 1u;
        Check(address == 0x9000u + 2836u + 0xa48u,
              "player action field offset");
        return action;
    }
    std::int32_t PlayerStat(std::uint32_t state,
                            std::uint32_t index) override {
        Check(state == 0x9000u + 2836u + 0x654u && index == 8u,
              "resource stat address and index");
        calls.emplace_back("stat");
        return resource;
    }
    void AddPlayerStat(std::uint32_t state,
                       std::uint32_t index,
                       std::int32_t amount) override {
        Check(state == 0x9000u + 2836u + 0x654u && index == 8u,
              "resource debit address and index");
        spent = amount;
        calls.emplace_back("debit");
    }
    std::uint32_t Arena() override {
        calls.emplace_back("arena");
        return 0xa000u;
    }
    void PlayerAction(std::uint32_t arena,
                      std::uint32_t selected) override {
        Check(arena == 0xa000u && selected == action,
              "arena and player action identity");
        acted = true;
        calls.emplace_back("action");
    }
    void PlayerActionSecondary(std::uint32_t arena,
                               std::uint32_t selected) override {
        Check(arena == 0xa000u && selected == action,
              "secondary arena action identity");
        secondary_acted = true;
        calls.emplace_back("secondary-action");
    }
    std::uint32_t TriggerItemText(std::uint32_t item) override {
        Check(item == 0x6cu, "insufficient-resource GameDB item");
        calls.emplace_back("trigger-text");
        return text;
    }
    std::uint32_t EmptyString() override {
        calls.emplace_back("empty");
        return 0xe000u;
    }
    std::uint32_t ConvertToUtf8(std::uint32_t source) override {
        converted_from = source;
        calls.emplace_back("utf8");
        return 0xf000u;
    }
    void ShowHint(std::uint32_t utf8,
                  std::uint32_t x_bits,
                  std::uint32_t y_bits,
                  std::uint32_t scale_bits,
                  std::uint32_t flag,
                  std::uint32_t enabled) override {
        Check(utf8 == 0xf000u && x_bits == 0x3f000000u &&
                  y_bits == 0x3e19999au && scale_bits == 0x40000000u &&
                  flag == 0u && enabled == 1u,
              "hint coordinates, scale and flags");
        calls.emplace_back("hint");
    }
};

void CheckSufficientResources() {
    for (const auto [action, cost] :
         {std::pair{0u, 15}, std::pair{1u, 5},
          std::pair{2u, 20}, std::pair{3u, 5}}) {
        Runtime runtime;
        runtime.action = action;
        runtime.resource = cost;
        Check(pal4::MinimapSceneUiSkillAction::Activate(
                  0xffffffffu, runtime) == 1u &&
                  runtime.spent == -cost && runtime.acted &&
                  runtime.calls == std::vector<std::string>{
                                       "manager", "read", "read", "stat",
                                       "debit", "arena", "action"},
              "threshold resource is debited before selected arena action");
    }
}

void CheckInsufficientResources() {
    Runtime runtime;
    runtime.action = 2u;
    runtime.resource = 19;
    Check(pal4::MinimapSceneUiSkillAction::Activate(
              0u, runtime) == 1u &&
              !runtime.acted && runtime.spent == 0 &&
              runtime.converted_from == 0xd000u &&
              runtime.calls == std::vector<std::string>{
                                   "manager", "read", "read", "stat",
                                   "trigger-text", "utf8", "hint"},
          "insufficient resource shows GameDB hint without debit");

    Runtime empty;
    empty.text = 0u;
    empty.resource = -1;
    (void)pal4::MinimapSceneUiSkillAction::Activate(0u, empty);
    Check(empty.converted_from == 0xe000u &&
              empty.calls[empty.calls.size() - 3u] == "empty",
          "null GameDB text uses global empty string before UTF-8 conversion");
}

void CheckStateUpdate() {
    Runtime runtime;
    runtime.action = 3u;
    Check(pal4::MinimapSceneUiSkillAction::UpdateState(
              0xffffffffu, runtime) == 1u &&
              runtime.secondary_acted && !runtime.acted &&
              runtime.calls == std::vector<std::string>{
                                   "manager", "read", "read", "arena",
                                   "secondary-action"},
          "state update dispatches current action through secondary arena path");

    Runtime missing;
    missing.null_manager = true;
    Check(pal4::MinimapSceneUiSkillAction::UpdateState(
              0u, missing) == 1u && !missing.secondary_acted &&
              missing.calls == std::vector<std::string>{"manager", "read"},
          "zero computed player skips secondary action");
}

}  // namespace

int main() {
    CheckSufficientResources();
    CheckInsufficientResources();
    CheckStateUpdate();
}
