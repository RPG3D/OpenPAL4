#include "pal4/skill_data_table.h"
#include "pal4/stunt_data_table.h"
#include "pal4/magic_data_table.h"
#include "pal4/story_data_table.h"
#include "pal4/story_desc_data_table.h"
#include "pal4/game_question_data_table.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataTableBindingRefreshRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::string fail_on;
    std::uint32_t release_result = 0x12345678;

    void Record(std::string event) {
        events.push_back(event);
        if (event == fail_on) throw std::runtime_error("binding failed");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t name) override {
        Check(name == 0x9000 + 52, "log uses table name slot");
        Record("log:" + std::string(prefix));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x9000 + 108 &&
                  source == 0x9000 + 20,
              "Effect assignment uses bound and raw fields");
        Record("assign");
    }
    void ProcessPropertyRegistry(std::uint32_t binding) override {
        Check(binding == 0x9000 + 108,
              "property registry uses bound fields");
        Record("registry");
    }
    void BindEffects(std::uint32_t binding) override {
        Check(binding == 0x9000 + 108, "bind uses bound fields");
        Record("bind");
    }
    std::uint32_t ReleaseLogBuffer() override {
        Record("release");
        return release_result;
    }
};

using RefreshFunction = std::uint32_t (*)(
    std::uint32_t, pal4::IGameDataTableBindingRefreshRuntime&);

Runtime Mismatched() {
    Runtime runtime;
    runtime.words[0x9000 + 128] = 0x6000;
    runtime.words[0x9000 + 132] = 0x6000 + 2 * 4;
    runtime.words[0x9000 + 24] = 0x5000;
    runtime.words[0x9000 + 28] = 0x5000 + 3 * 60;
    return runtime;
}

void CheckRebind(RefreshFunction refresh) {
    Runtime runtime = Mismatched();
    Check(refresh(0x9000, runtime) == runtime.release_result &&
              runtime.events == std::vector<std::string>{
                  "log:beginBindField, table:", "assign", "registry",
                  "bind", "log:endBindField, table:", "release"},
          "count mismatch rebinds between ordered log messages");
}

void CheckMatchingCounts(RefreshFunction refresh) {
    Runtime runtime = Mismatched();
    runtime.words[0x9000 + 132] = 0x6000 + 3 * 4;
    Check(refresh(0x9000, runtime) == 0 && runtime.events.empty(),
          "matching populated counts skip binding");

    Runtime empty;
    Check(refresh(0x9000, empty) == 0x9000 + 20 &&
              empty.events.empty(),
          "matching null counts retain table-plus-20 result");

    Runtime negative;
    negative.words[0x9000 + 128] = 0x6000;
    negative.words[0x9000 + 132] = 0x6000 - 4;
    negative.words[0x9000 + 24] = 0x5000;
    negative.words[0x9000 + 28] = 0x5000 - 60;
    Check(refresh(0x9000, negative) == 1 &&
              negative.events.empty(),
          "matching negative counts preserve native signed residue");
}

void CheckFailures(RefreshFunction refresh) {
    for (const auto& stage : std::vector<std::string>{
             "log:beginBindField, table:", "assign", "registry",
             "bind", "log:endBindField, table:"}) {
        Runtime runtime = Mismatched();
        runtime.fail_on = stage;
        try {
            static_cast<void>(refresh(0x9000, runtime));
            Check(false, "binding stage failure must propagate");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) == "binding failed" &&
                      runtime.events.size() >= 2 &&
                      runtime.events[runtime.events.size() - 2] == stage &&
                      runtime.events.back() == "release",
                  "every failed stage releases temporary log buffer");
        }
    }
}

}  // namespace

int main() {
    try {
        for (const auto refresh : {
                 pal4::SkillDataTable::RefreshBindings,
                 pal4::StuntDataTable::RefreshBindings,
                 pal4::MagicDataTable::RefreshBindings,
                 pal4::StoryDataTable::RefreshBindings,
                 pal4::StoryDescDataTable::RefreshBindings,
                 pal4::GameQuestionDataTable::RefreshBindings}) {
            CheckRebind(refresh);
            CheckMatchingCounts(refresh);
            CheckFailures(refresh);
        }
        std::cout << "game_data_table_binding_refresh_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
