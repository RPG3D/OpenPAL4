#include "pal4/script_global_callbacks.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptGlobalCallbackRuntime {
    std::uint32_t global_calls = 0u;
    std::uint32_t game_calls = 0u;
    std::uint32_t movie_bits = 0x80000000u;
    std::uint32_t quest_base = 0xfffffff0u;
    std::uint32_t quest_written = 0u;
    std::vector<std::string> calls;

    std::uint32_t GlobalData() override {
        calls.emplace_back("global");
        return 0x1000u + global_calls++ * 0x1000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x1018u, "movie flag word at global +0x18");
        calls.emplace_back("read"); return movie_bits;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x1018u, "movie flag write address");
        calls.emplace_back("write"); movie_bits = value;
    }
    void SaveDialogSettings(std::uint32_t global) override {
        Check(global == 0x2000u, "save uses refreshed global singleton");
        calls.emplace_back("save");
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game");
        return 0x3000u + game_calls++ * 0x1000u;
    }
    std::uint32_t GetGameDataEntry(std::uint32_t game,
                                   std::uint32_t index) override {
        Check(game == 0x3000u && index == 7u,
              "quest base comes from game data entry seven");
        calls.emplace_back("get:7"); return quest_base;
    }
    void SetGameDataEntry(std::uint32_t game, std::uint32_t index,
                          std::uint32_t value) override {
        Check(game == 0x4000u && index == 71u,
              "quest result goes to game data entry 71");
        calls.emplace_back("set:71"); quest_written = value;
    }
};

void MovieFlagBounds() {
    for (const auto bit : {0u, 1u, 30u, 31u}) {
        Runtime runtime;
        pal4::ScriptGlobalCallbacks::OpenMovieFlag(bit, runtime);
        Check(runtime.movie_bits ==
                  (0x80000000u | (1u << bit)) &&
              runtime.calls == std::vector<std::string>{
                  "global", "read", "write", "global", "save"},
              "valid movie flag sets one bit and saves settings");
    }
    for (const auto bit : {32u, 0xffffffffu}) {
        Runtime runtime;
        pal4::ScriptGlobalCallbacks::OpenMovieFlag(bit, runtime);
        Check(runtime.movie_bits == 0x80000000u &&
              runtime.calls == std::vector<std::string>{"global"},
              "invalid bit still obtains global data but does not save");
    }
}

void QuestDelta() {
    for (const auto delta : {0, 32, -20}) {
        Runtime runtime;
        pal4::ScriptGlobalCallbacks::AddQuestCompletionPercentage(
            delta, runtime);
        Check(runtime.quest_written ==
                  0xfffffff0u + static_cast<std::uint32_t>(delta) &&
              runtime.calls == std::vector<std::string>{
                  "game", "get:7", "game", "set:71"},
              "signed quest delta wraps at 32 bits and regets game instance");
    }
}

}  // namespace

int main() {
    try {
        MovieFlagBounds();
        QuestDelta();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
