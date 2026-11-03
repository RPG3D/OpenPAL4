#include "pal4/script_player_skill_acquisition.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerSkillAcquisitionRuntime {
    std::uint32_t player = 0x3000u;
    std::uint32_t window = 0x2000u;
    std::uint32_t record = 0x9000u;
    std::uint8_t has_skill = 1u;
    bool throw_show = false;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager"); return 0x1000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x1000u && index == -3,
              "signed player index forwarded");
        calls.emplace_back("player"); return player;
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("frames"); return 0x5000u;
    }
    std::uint32_t CreateHintWindow(std::uint32_t frames) override {
        Check(frames == 0x5000u, "hint frame owner");
        calls.emplace_back("window"); return window;
    }
    std::uint32_t GameDatabase() override {
        calls.emplace_back("database"); return 0x4000u;
    }
    std::uint8_t HasSkill(std::uint32_t table,
                          std::uint32_t id) override {
        Check(table == 0x400cu && id == 0x72u,
              "skill lookup table and ID");
        calls.emplace_back("has"); return has_skill;
    }
    void InsertSkill(std::uint32_t collection,
                     std::uint32_t id) override {
        Check(collection == player + 0x938u && id == 0x72u,
              "player sorted skill collection offset");
        calls.emplace_back("insert");
    }
    std::uint32_t FindSkill(std::uint32_t table,
                            std::uint32_t id) override {
        Check(table == 0x400cu && id == 0x72u,
              "second skill lookup table");
        calls.emplace_back("find"); return record;
    }
    void InitializeText() override {
        calls.emplace_back("init text");
    }
    std::string_view SkillName(std::uint32_t resolved_record) override {
        Check(resolved_record == record, "skill name record");
        calls.emplace_back("skill name"); return "Skyfire";
    }
    std::string_view PlayerName(std::uint32_t resolved_player) override {
        Check(resolved_player == player, "player name source");
        calls.emplace_back("player name"); return "Yun";
    }
    std::string_view GameString(std::uint32_t table,
                                std::uint32_t id) override {
        Check(table == 0x4034u && id == 0x118u,
              "learned-skill format table and ID");
        calls.emplace_back("format lookup"); return "%s learned %s";
    }
    void FormatText(std::string_view format,
                    std::string_view resolved_player,
                    std::string_view skill) override {
        Check(format == "%s learned %s" && resolved_player == "Yun" &&
                  skill == "Skyfire", "format player and skill names");
        calls.emplace_back("format");
    }
    std::string_view HintText() override {
        calls.emplace_back("text"); return "Yun learned Skyfire";
    }
    void ShowHint(std::uint32_t resolved_window,
                  std::string_view text, float x, float y, float scale,
                  std::uint32_t mode, std::uint32_t enabled) override {
        Check(resolved_window == window && text == "Yun learned Skyfire" &&
                  x == 0.5f && y == 0.15f && scale == 1.0f &&
                  mode == 0u && enabled == 1u,
              "hint geometry and text");
        calls.emplace_back("show");
        if (throw_show) throw std::runtime_error("show failed");
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio"); return 0xa000u;
    }
    void PlaySound(std::uint32_t audio, std::uint8_t flag,
                   std::string_view path, std::uint32_t mode) override {
        Check(audio == 0xa000u && flag == 2u && mode == 1u &&
                  path == "gamedata\\PALSound\\WA17.wav",
              "learned skill audio path and script flag");
        calls.emplace_back("sound");
    }
    std::uint8_t CleanupText() noexcept override {
        calls.emplace_back("cleanup"); return 0x7eu;
    }
};

void EarlyExits() {
    Runtime missing_player;
    missing_player.player = 0u;
    Check(pal4::ScriptPlayerSkillAcquisition::Add(
                  -3, 0x72u, 2u, missing_player) == 0u &&
              missing_player.calls == std::vector<std::string>{
                  "manager", "player"},
          "missing player skips window and skill lookup");
    Runtime missing_window;
    missing_window.window = 0u;
    Check(pal4::ScriptPlayerSkillAcquisition::Add(
                  -3, 0x72u, 2u, missing_window) == 0u &&
              missing_window.calls == std::vector<std::string>{
                  "manager", "player", "frames", "window"},
          "missing hint window stops before database lookup");
    Runtime invalid;
    invalid.has_skill = 0u;
    Check(pal4::ScriptPlayerSkillAcquisition::Add(
                  -3, 0x72u, 2u, invalid) == 0u &&
              invalid.calls == std::vector<std::string>{
                  "manager", "player", "frames", "window",
                  "database", "has"},
          "invalid skill never enters player collection");
}

void LearnedBranches() {
    Runtime quiet;
    Check(pal4::ScriptPlayerSkillAcquisition::Add(
                  -3, 0x72u, 0u, quiet) == 0u &&
              quiet.calls == std::vector<std::string>{
                  "manager", "player", "frames", "window",
                  "database", "has", "insert", "database", "find"},
          "quiet learn still inserts and resolves skill");
    Runtime missing_record;
    missing_record.record = 0u;
    Check(pal4::ScriptPlayerSkillAcquisition::Add(
                  -3, 0x72u, 2u, missing_record) == 2u &&
              missing_record.calls == quiet.calls,
          "missing record returns original hint byte after insertion");
    Runtime shown;
    shown.has_skill = 2u;
    Check(pal4::ScriptPlayerSkillAcquisition::Add(
                  -3, 0x72u, 2u, shown) == 0x7eu &&
              shown.calls == std::vector<std::string>{
                  "manager", "player", "frames", "window", "database",
                  "has", "insert", "database", "find", "init text",
                  "skill name", "player name", "database", "format lookup",
                  "format", "text", "show", "audio", "sound", "cleanup"},
          "hint path uses third database access and cleanup result");
}

void ExceptionCleanup() {
    Runtime runtime;
    runtime.throw_show = true;
    bool thrown = false;
    try {
        (void)pal4::ScriptPlayerSkillAcquisition::Add(
            -3, 0x72u, 2u, runtime);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && runtime.calls.size() >= 2u &&
              runtime.calls[runtime.calls.size() - 2u] == "show" &&
              runtime.calls.back() == "cleanup",
          "formatted text cleaned when hint display throws");
}

}  // namespace

int main() {
    try {
        EarlyExits();
        LearnedBranches();
        ExceptionCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
