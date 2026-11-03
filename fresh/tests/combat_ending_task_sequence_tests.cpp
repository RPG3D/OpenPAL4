#include "pal4/combat_ending_task_sequence.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
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

struct Runtime final : pal4::ICombatEndingTaskSequenceRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Task> tasks;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> main_actors;
    std::vector<std::uint32_t> candidates;
    std::size_t main_cursor = 0u;
    std::size_t candidate_cursor = 0u;
    bool inactive = false;
    bool found_attribute = true;
    bool fail_close_task = false;

    Runtime() {
        words[kDialog + 80u] = 800u;
        words[kDialog + 84u] = 840u;
        words[kDialog + 72u] = 720u;
        words[kDialog + 76u] = 760u;
        words[kDialog + 88u] = 880u;
        words[kDialog + 436u] = 0x7777u;
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            const auto row = kDialog + 116u + 76u * i;
            words[row] = 10000u + 76u * i;
            words[row + 60u] = std::bit_cast<std::uint32_t>(
                14.0f + static_cast<float>(i));
            words[kDialog + 684u + 4u * i] = 600u + i;
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
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x2cu, "task container size");
        calls.emplace_back("allocate");
        return kContainer;
    }
    void InitializeContainer(std::uint32_t address) override {
        Check(address == kContainer + 4u, "container list offset");
        calls.emplace_back("init-container");
    }
    void InitializeName(std::uint32_t address,
                        const char* text) override {
        Check(address == kContainer + 20u &&
                  std::string(text) == "EndTask", "container name");
        calls.emplace_back("init-name");
    }
    bool CombatInactive() override {
        calls.emplace_back("combat-state");
        return inactive;
    }
    std::string GlobalName(std::uint32_t address) override {
        const std::map<std::uint32_t, std::string> names{
            {0x8f94f4u, "uiShowScoreText"},
            {0x8f94e0u, "uiShowScoreLvText"},
            {0x8f956cu, "uiShowWindowTask"},
            {0x8f9530u, "uiMoveDownWindowTask"},
            {0x8f9544u, "uiUpdateExpTask"},
            {0x8f9580u, "uiSetupShowStep"},
            {0x8f9508u, "uiShowPrizePropTask"},
            {0x8f9558u, "uiCloseSystemTask"}};
        return names.at(address);
    }
    std::uint32_t CreateTask(pal4::CombatEndingTaskType type) override {
        if (fail_close_task &&
            type == pal4::CombatEndingTaskType::CloseSystem)
            throw std::runtime_error("factory failed");
        const auto address = 0xa0000u +
            static_cast<std::uint32_t>(tasks.size()) * 0x100u;
        tasks.push_back({type, address, {}, -1.0f});
        calls.emplace_back("create-task");
        return address;
    }
    void AddTask(std::uint32_t container, std::uint32_t task,
                 const std::string& name, std::int32_t mode,
                 float time, std::int32_t reserved) override {
        Check(container == kContainer + 4u && mode == 2 &&
                  reserved == 0 && tasks.back().address == task,
              "task container and timing header");
        tasks.back().name = name;
        tasks.back().time = time;
        calls.emplace_back("add-task");
    }
    std::uint32_t BeginActors(std::uint32_t query) override {
        Check(query == 37u || query == 38u, "actor query kind");
        calls.push_back("begin:" + std::to_string(query));
        return query == 37u ? 0xd0000u : 0xd1000u;
    }
    bool ActorsDone(std::uint32_t iterator) override {
        return iterator == 0xd0000u ?
            main_cursor == main_actors.size() :
            candidate_cursor == candidates.size();
    }
    std::uint32_t CurrentActor(std::uint32_t iterator) override {
        return iterator == 0xd0000u ? main_actors.at(main_cursor) :
                                       candidates.at(candidate_cursor);
    }
    void AdvanceActor(std::uint32_t iterator) override {
        if (iterator == 0xd0000u) ++main_cursor;
        else ++candidate_cursor;
    }
    int ReleaseActors(std::uint32_t iterator) override {
        calls.push_back("release:" + std::to_string(iterator));
        return iterator == 0xd0000u ? 137 : 138;
    }
    std::int32_t ActorStat(std::uint32_t actor,
                           std::uint32_t stat) override {
        calls.push_back("stat:" + std::to_string(stat));
        if (stat == 4u) return actor == 0x31000u ? 99 : 10;
        return 3;
    }
    std::int32_t RequiredExperience(std::uint32_t player_key,
                                     std::int32_t next_level) override {
        Check(player_key == 1004u && next_level == 11,
              "experience lookup key and next level");
        return 100;
    }
    std::uint32_t ActorRoleKey(std::uint32_t actor) override {
        Check(actor == 0x30000u, "argument-free Player role getter");
        return 888u;
    }
    std::optional<pal4::CombatEndingAttributeRecord> FindAttribute(
        std::uint32_t id, std::int32_t previous_experience) override {
        Check(id == 888u && previous_experience == 40,
              "database receives role key and prior XP after the getter");
        if (!found_attribute) return std::nullopt;
        return pal4::CombatEndingAttributeRecord{1234u, 55};
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 880u && !visible, "prize panel hidden");
        calls.emplace_back("hide-prize");
    }
    void ResetFlags(std::uint32_t address) override {
        Check(address == kDialog + 44u, "dialog router reset");
        calls.emplace_back("reset-dialog");
    }
    void ResetActorManagerFlags() override {
        calls.emplace_back("reset-manager");
    }
    void RegisterSequence(std::uint32_t router,
                          std::uint32_t container) override {
        Check(router == kDialog + 44u && container == kContainer,
              "sequence registration");
        calls.emplace_back("register-sequence");
    }
    bool ActorStateNotThree(std::uint32_t actor) override {
        calls.push_back("state:" + std::to_string(actor));
        return actor != 0x40000u;
    }
    void ResetObjectProperties(std::uint32_t actor) override {
        calls.push_back("reset-actor:" + std::to_string(actor));
    }
    std::uint32_t CombatWinAction(std::uint32_t actor) override {
        calls.push_back("win-action:" + std::to_string(actor));
        return actor + 1u;
    }
    void RegisterActorEvent(std::uint32_t value) override {
        calls.push_back("actor-event:" + std::to_string(value));
    }
};

std::size_t Find(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0u; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

const Task& FindTask(const Runtime& runtime,
                     pal4::CombatEndingTaskType type,
                     std::size_t occurrence = 0u) {
    for (const auto& task : runtime.tasks) {
        if (task.type == type && occurrence-- == 0u) return task;
    }
    throw std::runtime_error("missing task");
}

float FloatWord(const Runtime& runtime, std::uint32_t address) {
    return std::bit_cast<float>(runtime.words.at(address));
}

void ActiveCombatSchedulesAllStages() {
    Runtime runtime;
    runtime.main_actors = {0x32000u, 0x30000u, 0x31000u};
    runtime.words[0x32000u + 0x4a8u] = 4u;
    runtime.words[0x30000u + 0x4a8u] = 3u;
    runtime.words[0x31000u + 0x4a8u] = 0u;
    runtime.words[kDialog + 620u] = 40u;
    runtime.words[kDialog + 624u] = 60u;
    runtime.words[kDialog + 452u] = 2u;
    runtime.words[kDialog + 468u] = 3u;
    runtime.candidates = {0x40000u, 0x41000u};

    const auto result = pal4::CombatEndingTaskSequence::Start(
        kDialog, runtime);
    Check(result == 137 && runtime.tasks.size() == 15u &&
              runtime.words[kDialog + 716u] == 0u &&
              runtime.words[kContainer] ==
                  pal4::CombatEndingTaskSequence::kContainerVtable &&
              runtime.words[kContainer + 36u] == 0u &&
              runtime.bytes[kContainer + 40u] == 1u,
          "container construction and return");
    const std::vector<float> times{1.0f, 2.0f, 4.0f, 4.0f, 4.5f,
        4.5f, 5.0f, 5.0f, 6.0f, 6.0f, 8.5f, 8.5f, 9.0f, 9.0f, 12.5f};
    for (std::size_t i = 0u; i < times.size(); ++i)
        Check(runtime.tasks[i].time == times[i], "task timing sequence");
    Check(FindTask(runtime, pal4::CombatEndingTaskType::ScoreText).name ==
              "uiShowScoreText" &&
              FindTask(runtime, pal4::CombatEndingTaskType::LevelText).name ==
                  "uiShowScoreLvText" &&
              FindTask(runtime, pal4::CombatEndingTaskType::CloseSystem).name ==
                  "uiCloseSystemTask", "named factory identities");
    const auto& move0 = FindTask(
        runtime, pal4::CombatEndingTaskType::MoveDownWindow);
    const auto& move1 = FindTask(
        runtime, pal4::CombatEndingTaskType::MoveDownWindow, 1u);
    Check(runtime.words[move0.address + 60u] == 10000u &&
              FloatWord(runtime, move0.address + 64u) == 14.0f &&
              runtime.words[move1.address + 60u] == 10076u &&
              FloatWord(runtime, move1.address + 64u) == 15.0f,
          "valid actors pack row tasks in encounter order");
    const auto& exp = FindTask(
        runtime, pal4::CombatEndingTaskType::UpdateExperience);
    Check(runtime.words[exp.address + 60u] == kDialog + 116u &&
              runtime.words[exp.address + 64u] == kDialog + 420u &&
              runtime.words[exp.address + 68u] == 0x30000u &&
              runtime.words[exp.address + 92u] == kDialog + 700u &&
              runtime.words[exp.address + 96u] == 600u &&
              runtime.words[exp.address + 88u] == 97u &&
              FloatWord(runtime, exp.address + 72u) == 40.0f &&
              FloatWord(runtime, exp.address + 76u) == 10.0f &&
              runtime.words[exp.address + 80u] == 1234u &&
              FloatWord(runtime, exp.address + 84u) == 14.0f,
          "experience task fields and optional attribute");
    const auto& property_show = FindTask(
        runtime, pal4::CombatEndingTaskType::Window, 3u);
    const auto& property_hide = FindTask(
        runtime, pal4::CombatEndingTaskType::Window, 4u);
    Check(runtime.words[property_show.address + 60u] == 760u &&
              runtime.bytes[property_show.address + 64u] == 1u &&
              runtime.bytes[property_show.address + 65u] == 1u &&
              runtime.words[property_hide.address + 60u] == 760u &&
              runtime.bytes[property_hide.address + 64u] == 0u,
          "conditional property panel sequence");
    const auto& close = FindTask(
        runtime, pal4::CombatEndingTaskType::CloseSystem);
    Check(runtime.words[close.address + 60u] == 0x7777u &&
              Find(runtime, "reset-dialog") <
                  Find(runtime, "register-sequence") &&
              Find(runtime, "register-sequence") <
                  Find(runtime, "reset-manager") &&
              Find(runtime, "hide-prize") <
                  Find(runtime, "register-sequence") &&
              Find(runtime, "state:262144") <
                  Find(runtime, "state:266240") &&
              Find(runtime, "reset-actor:266240") <
                  Find(runtime, "win-action:266240") &&
              Find(runtime, "win-action:266240") <
                  Find(runtime, "actor-event:266241") &&
              Find(runtime, "release:856064") <
                  Find(runtime, "release:851968"),
          "registration, actor processing and reverse iterator release");
}

void InactiveCombatAndNoOptionalPanels() {
    Runtime runtime;
    runtime.inactive = true;
    Check(pal4::CombatEndingTaskSequence::Start(kDialog, runtime) == 137 &&
              runtime.tasks.size() == 7u,
          "inactive combat has no score prelude");
    const std::vector<float> times{0.0f, 0.0f, 0.0f, 1.0f,
                                    3.0f, 3.5f, 5.5f};
    for (std::size_t i = 0u; i < times.size(); ++i)
        Check(runtime.tasks[i].time == times[i],
              "inactive timing and empty property/prize branches");
    Check(runtime.tasks.front().type ==
              pal4::CombatEndingTaskType::Window &&
              runtime.tasks.back().type ==
                  pal4::CombatEndingTaskType::CloseSystem,
          "inactive task types");
}

void MissingAttributeAndExceptionCleanup() {
    Runtime runtime;
    runtime.main_actors = {0x30000u};
    runtime.words[0x30000u + 0x4a8u] = 3u;
    runtime.words[kDialog + 620u] = 40u;
    runtime.words[kDialog + 624u] = 60u;
    runtime.found_attribute = false;
    (void)pal4::CombatEndingTaskSequence::Start(kDialog, runtime);
    const auto& exp = FindTask(
        runtime, pal4::CombatEndingTaskType::UpdateExperience);
    Check(runtime.words[exp.address + 80u] == 0u &&
              FloatWord(runtime, exp.address + 84u) == 40.0f,
          "missing attribute leaves initial experience fields");

    Runtime failing;
    failing.fail_close_task = true;
    bool threw = false;
    try {
        (void)pal4::CombatEndingTaskSequence::Start(kDialog, failing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failing.calls.back() == "release:851968",
          "first actor iterator released on later factory exception");
}

}  // namespace

int main() {
    try {
        ActiveCombatSchedulesAllStages();
        InactiveCombatAndNoOptionalPanels();
        MissingAttributeAndExceptionCleanup();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
