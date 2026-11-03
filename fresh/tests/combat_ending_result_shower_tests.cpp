#include "pal4/combat_ending_result_shower.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t kDialog = 0x10000u;
constexpr std::uint32_t kContainer = 0xc0000u;

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Task {
    pal4::CombatEndingTaskType type;
    std::uint32_t address;
    std::string name;
    float time = -1.0f;
};

struct Runtime final : pal4::ICombatEndingResultShowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, float> positions;
    std::vector<Task> tasks;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> actors;
    std::size_t cursor = 0u;
    bool no_iterator = false;
    bool fail_text = false;

    Runtime() {
        words[kDialog + 80u] = 800u;
        words[kDialog + 84u] = 840u;
        words[kDialog + 72u] = 720u;
        words[kDialog + 76u] = 760u;
        words[kDialog + 436u] = 0x7777u;
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            const auto row = kDialog + 116u + 76u * i;
            words[row] = 10000u + i;
            words[row + 20u] = 11000u + i;
            words[row + 24u] = 12000u + i;
            words[row + 60u] = std::bit_cast<std::uint32_t>(
                13.0f + static_cast<float>(i));
            words[kDialog + 684u + 4u * i] = 20000u + i;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void CleanupCombatConsole() override {
        calls.emplace_back("cleanup-console");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x2cu, "task container allocation size");
        calls.emplace_back("allocate");
        return kContainer;
    }
    void InitializeContainer(std::uint32_t address) override {
        Check(address == kContainer + 4u, "container list location");
        calls.emplace_back("init-container");
    }
    void InitializeName(std::uint32_t address,
                        const char* text) override {
        Check(address == kContainer + 20u &&
                  std::string(text) == "EndTask", "container text");
        calls.emplace_back("init-name");
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check((visible && (window == 800u || window == 840u)) ||
                  (!visible && (window == 720u || window == 760u)),
              "score or result panel visibility");
        calls.push_back("visible:" + std::to_string(window));
    }
    void SetYPosition(std::uint32_t window, float y) override {
        positions[window] = y;
        calls.push_back("position:" + std::to_string(window));
    }
    void SetText(std::uint32_t window,
                 std::string_view text) override {
        if (fail_text) throw std::runtime_error("text failed");
        texts[window] = text;
        calls.push_back("text:" + std::to_string(window));
    }
    std::uint32_t BeginActors() override {
        calls.emplace_back("begin");
        return no_iterator ? 0u : 0xd000u;
    }
    bool ActorsDone(std::uint32_t iterator) override {
        return iterator == 0u || cursor == actors.size();
    }
    std::uint32_t CurrentActor(std::uint32_t iterator) override {
        Check(iterator == 0xd000u, "current iterator");
        return actors.at(cursor);
    }
    void AdvanceActor(std::uint32_t iterator) override {
        Check(iterator == 0xd000u, "advance iterator");
        ++cursor;
    }
    int ReleaseActors(std::uint32_t iterator) override {
        Check(iterator == 0xd000u, "released iterator");
        calls.emplace_back("release");
        return 42;
    }
    std::int32_t ActorStat(std::uint32_t actor,
                           std::uint32_t stat) override {
        calls.push_back("stat:" + std::to_string(stat));
        if (stat == 4u) return actor == 0x31000u ? 99 : 10;
        return 50;
    }
    std::int32_t RequiredExperience(std::uint32_t player_key,
                                     std::int32_t next_level) override {
        Check(player_key == 1004u && next_level == 11,
              "next-level GameDB query");
        return 1000;
    }
    void HandleLevelUp(std::uint32_t row, std::int32_t level,
                       std::uint32_t actor, std::uint32_t hint,
                       std::uint32_t record) override {
        const auto index = actor == 0x30000u ? 0u : 1u;
        Check(row == kDialog + 116u + 76u * index &&
                  level == (index == 0u ? 10 : 99) &&
                  hint == 20000u + index &&
                  record == kDialog + 472u +
                      40u * (index == 0u ? 3u : 0u),
              "level-up row, actor, hint and record");
        calls.push_back("level-up:" + std::to_string(actor));
    }
    std::string GlobalName(std::uint32_t address) override {
        const std::map<std::uint32_t, std::string> names{
            {0x8f956cu, "uiShowWindowTask"},
            {0x8f9580u, "uiSetupShowStep"},
            {0x8f9508u, "uiShowPrizePropTask"},
            {0x8f9558u, "uiCloseSystemTask"}};
        return names.at(address);
    }
    std::uint32_t CreateTask(pal4::CombatEndingTaskType type) override {
        const auto address = 0xa0000u +
            static_cast<std::uint32_t>(tasks.size()) * 0x100u;
        tasks.push_back({type, address, {}, -1.0f});
        calls.emplace_back("create-task");
        return address;
    }
    void AddTask(std::uint32_t container, std::uint32_t task,
                 const std::string& name, std::int32_t mode,
                 float time, std::int32_t reserved) override {
        Check(container == kContainer + 4u &&
                  task == tasks.back().address && mode == 2 &&
                  reserved == 0, "task base timing and list");
        tasks.back().name = name;
        tasks.back().time = time;
        calls.emplace_back("add-task");
    }
    void ResetFlags(std::uint32_t address) override {
        Check(address == kDialog + 44u, "dialog router reset");
        calls.emplace_back("reset");
    }
    int RegisterSequence(std::uint32_t router,
                          std::uint32_t container) override {
        Check(router == kDialog + 44u && container == kContainer,
              "sequence registration");
        calls.emplace_back("register");
        return 17;
    }
};

std::size_t Find(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0u; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

void ActiveRowsAndOptionalTaskPhases() {
    Runtime runtime;
    runtime.actors = {0x32000u, 0x30000u, 0x31000u};
    runtime.words[0x32000u + 0x4a8u] = 4u;
    runtime.words[0x30000u + 0x4a8u] = 3u;
    runtime.words[0x31000u + 0x4a8u] = 0u;
    runtime.words[kDialog + 620u] = 20u;
    runtime.words[kDialog + 452u] = 2u;
    runtime.words[kDialog + 468u] = 3u;

    Check(pal4::CombatEndingResultShower::Show(kDialog, runtime) == 42 &&
              runtime.tasks.size() == 7u &&
              runtime.words[kDialog + 716u] == 1u &&
              runtime.words[kContainer] ==
                  pal4::CombatEndingTaskSequence::kContainerVtable &&
              runtime.words[kContainer + 36u] == 0u &&
              runtime.bytes[kContainer + 40u] == 1u,
          "combat result setup and iterator result");
    Check(runtime.positions[10000u] == 13.0f &&
              runtime.positions[10001u] == 14.0f &&
              runtime.texts[11000u] == "30" &&
              runtime.texts[12000u] == "950" &&
              !runtime.texts.contains(11001u) &&
              runtime.cursor == 3u,
          "packed row Y, XP delta, next XP and level gate");
    const std::vector<float> times{0.0f, 0.0f, 2.5f, 2.5f,
                                    3.0f, 3.0f, 6.5f};
    for (std::size_t i = 0u; i < times.size(); ++i)
        Check(runtime.tasks[i].time == times[i],
              "optional panel and close schedule");
    Check(runtime.tasks[0].type ==
              pal4::CombatEndingTaskType::Window &&
              runtime.tasks[4].type ==
                  pal4::CombatEndingTaskType::ShowStep &&
              runtime.tasks[5].type ==
                  pal4::CombatEndingTaskType::PrizeProperty &&
              runtime.tasks[6].type ==
                  pal4::CombatEndingTaskType::CloseSystem &&
              runtime.words[runtime.tasks[1].address + 60u] == 760u &&
              runtime.bytes[runtime.tasks[1].address + 65u] == 1u &&
              runtime.words[runtime.tasks[6].address + 60u] == 0x7777u,
          "task types and target fields");
    Check(Find(runtime, "cleanup-console") <
              Find(runtime, "word:" + std::to_string(kDialog + 716u)) &&
              Find(runtime, "visible:800") <
                  Find(runtime, "visible:840") &&
              Find(runtime, "level-up:196608") <
                  Find(runtime, "level-up:200704") &&
              Find(runtime, "level-up:200704") <
                  Find(runtime, "create-task") &&
              Find(runtime, "reset") < Find(runtime, "register") &&
              Find(runtime, "register") < Find(runtime, "release"),
          "console, rows, task and registration order");
}

void EmptyOptionalBranchesAndNoIterator() {
    Runtime runtime;
    runtime.no_iterator = true;
    Check(pal4::CombatEndingResultShower::Show(kDialog, runtime) == 17 &&
              runtime.tasks.size() == 4u &&
              runtime.tasks[0].time == 0.0f &&
              runtime.tasks[1].time == 2.0f &&
              runtime.tasks[2].time == 2.5f &&
              runtime.tasks[3].time == 4.5f &&
              runtime.calls.back() == "register",
          "empty property/prize branches and register return");
}

void TextFailureReleasesIterator() {
    Runtime runtime;
    runtime.actors = {0x30000u};
    runtime.words[0x30000u + 0x4a8u] = 3u;
    runtime.words[kDialog + 620u] = 20u;
    runtime.fail_text = true;
    bool failed = false;
    try {
        (void)pal4::CombatEndingResultShower::Show(kDialog, runtime);
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.calls.back() == "release",
          "iterator released after CEGUI text exception");
}

void HideWithoutPrize() {
    Runtime runtime;
    Check(pal4::CombatEndingResultShower::Hide(kDialog, runtime) == 17 &&
              runtime.words[kDialog + 716u] == 2u &&
              runtime.tasks.size() == 1u &&
              runtime.tasks[0].type ==
                  pal4::CombatEndingTaskType::CloseSystem &&
              runtime.tasks[0].time == 2.0f &&
              runtime.words[runtime.tasks[0].address + 60u] == 0x7777u &&
              Find(runtime, "cleanup-console") <
                  Find(runtime, "word:" + std::to_string(kDialog + 716u)) &&
              Find(runtime, "visible:720") <
                  Find(runtime, "visible:760") &&
              Find(runtime, "visible:760") <
                  Find(runtime, "create-task") &&
              Find(runtime, "reset") < Find(runtime, "register"),
          "hide clears console, hides panels and schedules close");
}

void HideWithPrize() {
    Runtime runtime;
    runtime.words[kDialog + 468u] = 3u;
    Check(pal4::CombatEndingResultShower::Hide(kDialog, runtime) == 17 &&
              runtime.tasks.size() == 2u &&
              runtime.tasks[0].type ==
                  pal4::CombatEndingTaskType::PrizeProperty &&
              runtime.tasks[0].name == "uiShowPrizePropTask" &&
              runtime.tasks[0].time == 0.0f &&
              runtime.words[runtime.tasks[0].address + 60u] == kDialog &&
              runtime.tasks[1].type ==
                  pal4::CombatEndingTaskType::CloseSystem &&
              runtime.tasks[1].time == 3.5f &&
              runtime.words[runtime.tasks[1].address + 60u] == 0x7777u,
          "prize count adds half-second work before close");
}

}  // namespace

int main() {
    try {
        ActiveRowsAndOptionalTaskPhases();
        EmptyOptionalBranchesAndNoIterator();
        TextFailureReleasesIterator();
        HideWithoutPrize();
        HideWithPrize();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
