#include "pal4/script_monster_visibility.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptMonsterVisibilityRuntime {
    std::uint32_t name = 0x7000u;
    std::uint32_t monster = 0x3000u;
    std::uint32_t component = 0x9000u;
    std::uint32_t info = 0xa000u;
    std::string animation = "A01";
    std::uint32_t c01_result = 1u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x5004u) {
            calls.emplace_back("name"); return name;
        }
        Check(address == monster + 0x654u, "monster component offset");
        calls.emplace_back("component"); return component;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty"); return 0x6000u;
    }
    std::uint32_t MonsterManager() override {
        calls.emplace_back("manager"); return 0x2000u;
    }
    std::uint32_t FindMonster(std::uint32_t manager,
                              std::uint32_t resolved_name) override {
        Check(manager == 0x2000u && resolved_name ==
                  (name == 0u ? 0x6000u : name), "monster lookup operands");
        calls.emplace_back("find"); return monster;
    }
    std::uint32_t FindMonsterInfo(std::uint32_t manager,
                                  std::uint32_t resolved_name) override {
        Check(manager == 0x2000u && resolved_name ==
                  (name == 0u ? 0x6000u : name),
              "secondary monster info lookup operands");
        calls.emplace_back("info"); return info;
    }
    std::string_view AnimationName(std::uint32_t address) override {
        Check(address == monster + 0x170u, "animation offset");
        calls.emplace_back("animation"); return animation;
    }
    std::int32_t CompareIgnoreCase(std::string_view lhs,
                                   std::string_view rhs) override {
        Check(lhs == animation && (rhs == "Z01" || rhs == "C01"),
              "compare current animation with requested idle names");
        calls.emplace_back(std::string("compare:") + std::string(rhs));
        auto upper = std::string(lhs);
        for (auto& ch : upper) {
            if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
        }
        return upper == rhs ? 0 : 1;
    }
    std::uint32_t PlayAnimation(std::uint32_t address,
                                std::string_view requested) override {
        Check(address == monster + 0x170u, "play animation component");
        calls.emplace_back(std::string("play:") + std::string(requested));
        return requested == "C01" ? c01_result : 1u;
    }
    void SetStateFlag(std::uint32_t object,
                      std::uint32_t state) override {
        Check(object == monster && state <= 1u, "monster state flag");
        calls.emplace_back("state:" + std::to_string(state));
    }
    void ReplaceComponent(std::uint32_t object, std::uint32_t value,
                          std::uint32_t enabled) override {
        Check(object == monster && (value == component || value == info) &&
                  enabled == 1u,
              "present component replacement");
        calls.emplace_back("replace");
    }
    void CleanupBuffers(std::uint32_t object,
                        std::uint32_t mode) override {
        Check(object == monster && mode == 1u, "cleanup mode one");
        calls.emplace_back("cleanup");
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena"); return 0x8000u;
    }
    std::uint32_t EnterCharacter(std::uint32_t arena,
                                 std::uint32_t object) override {
        Check(arena == 0x8000u && object == monster, "arena add operands");
        calls.emplace_back("enter"); return 0x81u;
    }
    std::uint32_t LeaveCharacter(std::uint32_t arena,
                                 std::uint32_t object) override {
        Check(arena == 0x8000u && object == monster, "arena remove operands");
        calls.emplace_back("leave"); return 0x82u;
    }
};

void MissingAndUnhide() {
    Runtime missing;
    missing.monster = 0u;
    Check(pal4::ScriptMonsterVisibility::SetHide(0x5000u, 1u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "name", "manager", "find"},
          "missing monster leaves state unchanged");
    Runtime shown;
    shown.name = 0u;
    Check(pal4::ScriptMonsterVisibility::SetHide(0x5000u, 0u, shown) == 0x82u &&
              shown.calls == std::vector<std::string>{
                  "name", "empty", "manager", "find", "state:0",
                  "cleanup", "arena", "leave"},
          "zero hide flag clears state then removes character");
}

void HideAnimations() {
    for (const auto current : {"z01", "C01", "A01"}) {
        Runtime runtime;
        runtime.animation = current;
        auto expected = std::vector<std::string>{
            "name", "manager", "find", "animation", "compare:Z01"};
        if (runtime.animation != "z01") {
            expected.insert(expected.end(), {"animation", "compare:C01"});
            if (runtime.animation == "A01") expected.emplace_back("play:C01");
        }
        expected.insert(expected.end(), {"state:1", "component", "replace",
                                         "arena", "enter"});
        Check(pal4::ScriptMonsterVisibility::SetHide(0x5000u, 2u, runtime) ==
                  0x81u && runtime.calls == expected,
              "hide honors idle names and uses C01 on active animation");
    }
    Runtime fallback;
    fallback.c01_result = 0u;
    fallback.component = 0u;
    Check(pal4::ScriptMonsterVisibility::SetHide(0x5000u, 1u, fallback) ==
                  0x81u &&
              fallback.calls == std::vector<std::string>{
                  "name", "manager", "find", "animation", "compare:Z01",
                  "animation", "compare:C01", "play:C01", "play:Z01",
                  "state:1", "component", "arena", "enter"},
          "failed C01 tries Z01 and missing component is skipped");
}

void VisibleBranches() {
    Runtime missing;
    missing.monster = 0u;
    Check(pal4::ScriptMonsterVisibility::SetVisible(
                  0x5000u, 1u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "name", "manager", "find"},
          "missing monster skips secondary lookup and arena");
    Runtime hidden;
    hidden.name = 0u;
    Check(pal4::ScriptMonsterVisibility::SetVisible(
                  0x5000u, 0u, hidden) == 0x82u &&
              hidden.calls == std::vector<std::string>{
                  "name", "empty", "manager", "find", "name", "empty",
                  "manager", "info", "arena", "leave"},
          "visible false relooks up info then only leaves arena");
    Runtime shown;
    shown.animation = "C01";
    Check(pal4::ScriptMonsterVisibility::SetVisible(
                  0x5000u, 2u, shown) == 0x81u &&
              shown.calls == std::vector<std::string>{
                  "name", "manager", "find", "name", "manager", "info",
                  "arena", "leave", "animation", "compare:Z01",
                  "animation", "compare:C01", "state:1", "replace",
                  "arena", "enter"},
          "visible true removes first then sets idle/state/info and readds");
    Runtime fallback;
    fallback.animation = "A01";
    fallback.c01_result = 0u;
    fallback.info = 0u;
    Check(pal4::ScriptMonsterVisibility::SetVisible(
                  0x5000u, 1u, fallback) == 0x81u &&
              fallback.calls == std::vector<std::string>{
                  "name", "manager", "find", "name", "manager", "info",
                  "arena", "leave", "animation", "compare:Z01",
                  "animation", "compare:C01", "play:C01", "play:Z01",
                  "state:1", "arena", "enter"},
          "visible true uses Z01 fallback and skips missing info");
}

}  // namespace

int main() {
    try {
        MissingAndUnhide();
        HideAnimations();
        VisibleBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
