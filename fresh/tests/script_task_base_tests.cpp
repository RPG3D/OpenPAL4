#include "pal4/script_task_base.h"
#include "pal4/start_attack_task.h"
#include "pal4/physical_attack_task.h"
#include "pal4/quick_attack_task.h"
#include "pal4/cast_magic_sequence_task.h"
#include "pal4/quick_cast_magic_sequence_task.h"
#include "pal4/combat_stunt_sequence_task.h"
#include "pal4/combat_use_property_sequence_task.h"
#include "pal4/combat_defend_sequence_task.h"
#include "pal4/combat_flee_sequence_task.h"
#include "pal4/combat_dead_sequence_task.h"
#include "pal4/combat_wounded_sequence_task.h"
#include "pal4/combat_fend_off_sequence_task.h"
#include "pal4/combat_miss_sequence_task.h"
#include "pal4/combat_end_action_task.h"
#include "pal4/win_action_task.h"
#include "pal4/level_up_action_task.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x3000u;
constexpr std::uint32_t kText = 0x4001u;

struct Runtime final : pal4::IScriptTaskBaseRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 24u, kText}, {kTask + 28u, 12u},
        {kTask + 32u, 31u}};
    std::map<std::uint32_t, std::uint8_t> bytes{{kText - 1u, 0u}};
    std::vector<std::string> calls;
    std::uint32_t destroy_result = 0xabcd1234u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read_byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write_byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void CleanupObjectCollection(std::uint32_t container) override {
        Check(container == kTask + 4u, "object collection begins at +4");
        calls.push_back("cleanup_collection");
    }
    void Free(std::uint32_t address) override {
        Check(address == kText - 1u || address == kTask,
              "free receives text header or deleting task");
        calls.push_back(address == kTask ? "free_task" : "free");
    }
    std::uint32_t DestroyContainer(std::uint32_t container) override {
        Check(container == kTask + 4u, "GOM container begins at +4");
        calls.push_back("destroy_container");
        return destroy_result;
    }
};

std::size_t Index(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call occurred");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void VerifyCommon(const Runtime& runtime) {
    Check(runtime.words.at(kTask) == pal4::ScriptTaskBase::kVtable &&
          runtime.words.at(kTask + 24u) == 0u &&
          runtime.words.at(kTask + 28u) == 0u &&
          runtime.words.at(kTask + 32u) == 0u,
          "base destructor resets vtable and name header");
    Check(runtime.calls.front() == "write:12288" &&
          Index(runtime, "cleanup_collection") <
              Index(runtime, "read:12312") &&
          runtime.calls.back() == "destroy_container",
          "base cleanup runs in IDA order");
}

void ReleaseOwnedText() {
    for (const auto count : {0u, 0xffu}) {
        Runtime runtime;
        runtime.bytes[kText - 1u] = static_cast<std::uint8_t>(count);
        Check(pal4::ScriptTaskBase::Destroy(kTask, runtime) ==
                  runtime.destroy_result &&
              Index(runtime, "free") < Index(runtime, "write:12312"),
              "zero and sentinel reference bytes free text");
        VerifyCommon(runtime);
    }
}

void DecrementSharedText() {
    Runtime runtime;
    runtime.bytes[kText - 1u] = 3u;
    Check(pal4::ScriptTaskBase::Destroy(kTask, runtime) ==
              runtime.destroy_result &&
          runtime.bytes[kText - 1u] == 2u &&
          std::find(runtime.calls.begin(), runtime.calls.end(), "free") ==
              runtime.calls.end(),
          "shared text decrements reference byte without freeing");
    VerifyCommon(runtime);
}

void EmptyText() {
    Runtime runtime;
    runtime.words[kTask + 24u] = 0u;
    Check(pal4::ScriptTaskBase::Destroy(kTask, runtime) ==
              runtime.destroy_result &&
          std::find(runtime.calls.begin(), runtime.calls.end(), "free") ==
              runtime.calls.end(),
          "null text skips reference count and free");
    VerifyCommon(runtime);
}

void StartAttackBaseDestructor() {
    Runtime runtime;
    Check(pal4::StartAttackTask::DestroyBase(kTask, runtime) ==
              runtime.destroy_result &&
          Index(runtime, "free") < Index(runtime, "destroy_container"),
          "StartAttack base destructor delegates complete cleanup");
    VerifyCommon(runtime);
}

void PhysicalAttackBaseDestructor() {
    Runtime runtime;
    Check(pal4::PhysicalAttackTask::DestroyBase(kTask, runtime) ==
              runtime.destroy_result &&
          Index(runtime, "free") < Index(runtime, "destroy_container"),
          "PhysicalAttack base destructor delegates complete cleanup");
    VerifyCommon(runtime);
}

void EndAttackBaseDestructor() {
    Runtime runtime;
    Check(pal4::EndAttackTask::DestroyBase(kTask, runtime) ==
              runtime.destroy_result &&
          Index(runtime, "free") < Index(runtime, "destroy_container"),
          "EndAttack base destructor delegates complete cleanup");
    VerifyCommon(runtime);
}

void QuickAttackBaseDestructor() {
    Runtime runtime;
    Check(pal4::QuickAttackTask::DestroyBase(kTask, runtime) ==
              runtime.destroy_result &&
          Index(runtime, "free") < Index(runtime, "destroy_container"),
          "QuickAttack base destructor delegates complete cleanup");
    VerifyCommon(runtime);
}

void QuickAttackEndDestructors() {
    Runtime runtime;
    Check(pal4::QuickAttackEndTask::DestroyBase(kTask, runtime) ==
              runtime.destroy_result,
          "cbQuickEnd base destructor forwards GOM result");
    VerifyCommon(runtime);
    for (const auto flags : {0u, 1u, 2u, 3u}) {
        runtime = Runtime{};
        Check(pal4::QuickAttackEndTask::Destroy(
                  kTask, static_cast<std::uint8_t>(flags), runtime) ==
                  kTask &&
                  (std::find(runtime.calls.begin(), runtime.calls.end(),
                             "free_task") != runtime.calls.end()) ==
                      ((flags & 1u) != 0u),
              "deleting destructor frees task only for low flag bit");
        Check(runtime.calls.back() ==
                  ((flags & 1u) != 0u ? "free_task"
                                      : "destroy_container"),
              "task free follows complete base cleanup");
        if ((flags & 1u) == 0u) {
            VerifyCommon(runtime);
        } else {
            Check(Index(runtime, "cleanup_collection") <
                      Index(runtime, "destroy_container") &&
                      Index(runtime, "destroy_container") <
                          Index(runtime, "free_task") &&
                      runtime.words.at(kTask) ==
                          pal4::ScriptTaskBase::kVtable &&
                      runtime.words.at(kTask + 24u) == 0u &&
                      runtime.words.at(kTask + 28u) == 0u &&
                      runtime.words.at(kTask + 32u) == 0u,
                  "deleting destructor completes base cleanup before free");
        }
    }
}

void CastMagicDestructors() {
    for (bool quick : {false, true}) {
        Runtime runtime;
        const auto base_result = quick
            ? pal4::QuickCastMagicSequenceTask::DestroyBase(kTask, runtime)
            : pal4::CastMagicSequenceTask::DestroyBase(kTask, runtime);
        Check(base_result == runtime.destroy_result,
              "magic base destructor forwards GOM result");
        VerifyCommon(runtime);

        for (const auto flags : {0u, 1u, 2u, 3u}) {
            runtime = Runtime{};
            const auto result = quick
                ? pal4::QuickCastMagicSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime)
                : pal4::CastMagicSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime);
            Check(result == kTask &&
                      (std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "free_task") != runtime.calls.end()) ==
                          ((flags & 1u) != 0u),
                  "magic deleting destructor honors low flag bit");
            Check(Index(runtime, "cleanup_collection") <
                      Index(runtime, "destroy_container") &&
                      runtime.calls.back() == ((flags & 1u) != 0u
                          ? "free_task" : "destroy_container"),
                  "magic deletion follows complete base cleanup");
            if ((flags & 1u) == 0u) VerifyCommon(runtime);
        }
    }
}

void StuntDestructors() {
    for (bool quick : {false, true}) {
        Runtime runtime;
        const auto base_result = quick
            ? pal4::QuickStuntSequenceTask::DestroyBase(kTask, runtime)
            : pal4::StuntSequenceTask::DestroyBase(kTask, runtime);
        Check(base_result == runtime.destroy_result,
              "stunt base destructor forwards GOM result");
        VerifyCommon(runtime);

        for (const auto flags : {0u, 1u, 2u, 3u}) {
            runtime = Runtime{};
            const auto result = quick
                ? pal4::QuickStuntSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime)
                : pal4::StuntSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime);
            Check(result == kTask &&
                      (std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "free_task") != runtime.calls.end()) ==
                          ((flags & 1u) != 0u),
                  "stunt deleting destructor honors low flag bit");
            Check(Index(runtime, "cleanup_collection") <
                      Index(runtime, "destroy_container") &&
                      runtime.calls.back() == ((flags & 1u) != 0u
                          ? "free_task" : "destroy_container"),
                  "stunt deletion follows complete base cleanup");
            if ((flags & 1u) == 0u) VerifyCommon(runtime);
        }
    }
}

void UsePropertyDestructors() {
    for (bool quick : {false, true}) {
        Runtime runtime;
        const auto base_result = quick
            ? pal4::QuickUsePropertySequenceTask::DestroyBase(kTask, runtime)
            : pal4::UsePropertySequenceTask::DestroyBase(kTask, runtime);
        Check(base_result == runtime.destroy_result,
              "property base destructor forwards GOM result");
        VerifyCommon(runtime);

        for (const auto flags : {0u, 1u, 2u, 3u}) {
            runtime = Runtime{};
            const auto result = quick
                ? pal4::QuickUsePropertySequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime)
                : pal4::UsePropertySequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime);
            Check(result == kTask &&
                      (std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "free_task") != runtime.calls.end()) ==
                          ((flags & 1u) != 0u),
                  "property deleting destructor honors low flag bit");
            Check(Index(runtime, "cleanup_collection") <
                      Index(runtime, "destroy_container") &&
                      runtime.calls.back() == ((flags & 1u) != 0u
                          ? "free_task" : "destroy_container"),
                  "property deletion follows complete base cleanup");
            if ((flags & 1u) == 0u) VerifyCommon(runtime);
        }
    }
}

void DefendDestructors() {
    Runtime runtime;
    Check(pal4::DefendSequenceTask::DestroyBase(kTask, runtime) ==
              runtime.destroy_result,
          "Defend base destructor forwards GOM result");
    VerifyCommon(runtime);

    for (const auto flags : {0u, 1u, 2u, 3u}) {
        runtime = Runtime{};
        Check(pal4::DefendSequenceTask::Destroy(
                  kTask, static_cast<std::uint8_t>(flags), runtime) ==
                  kTask &&
                  (std::find(runtime.calls.begin(), runtime.calls.end(),
                             "free_task") != runtime.calls.end()) ==
                      ((flags & 1u) != 0u),
              "Defend deleting destructor honors low flag bit");
        Check(Index(runtime, "cleanup_collection") <
                  Index(runtime, "destroy_container") &&
                  runtime.calls.back() == ((flags & 1u) != 0u
                      ? "free_task" : "destroy_container"),
              "Defend deletion follows complete base cleanup");
        if ((flags & 1u) == 0u) VerifyCommon(runtime);
    }
}

void FleeDestructorsAndZeroCallback() {
    for (bool quick : {false, true}) {
        Runtime runtime;
        const auto base_result = quick
            ? pal4::QuickFleeSequenceTask::DestroyBase(kTask, runtime)
            : pal4::FleeSequenceTask::DestroyBase(kTask, runtime);
        Check(base_result == runtime.destroy_result,
              "Flee base destructor forwards GOM result");
        VerifyCommon(runtime);

        for (const auto flags : {0u, 1u, 2u, 3u}) {
            runtime = Runtime{};
            const auto result = quick
                ? pal4::QuickFleeSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime)
                : pal4::FleeSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime);
            Check(result == kTask &&
                      (std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "free_task") != runtime.calls.end()) ==
                          ((flags & 1u) != 0u),
                  "Flee deleting destructor honors low flag bit");
            Check(Index(runtime, "cleanup_collection") <
                      Index(runtime, "destroy_container") &&
                      runtime.calls.back() == ((flags & 1u) != 0u
                          ? "free_task" : "destroy_container"),
                  "Flee deletion follows complete base cleanup");
            if ((flags & 1u) == 0u) VerifyCommon(runtime);
        }
    }

    for (const auto value : {0u, 1u, 0xffffffffu}) {
        Check(pal4::ScriptTaskBase::ReturnZero(value) == 0u,
              "shared task callback returns zero for every argument");
    }
}

void DeadDestructors() {
    for (bool quick : {false, true}) {
        Runtime runtime;
        const auto base_result = quick
            ? pal4::QuickDeadSequenceTask::DestroyBase(kTask, runtime)
            : pal4::DeadSequenceTask::DestroyBase(kTask, runtime);
        Check(base_result == runtime.destroy_result,
              "Dead base destructor forwards GOM result");
        VerifyCommon(runtime);

        for (const auto flags : {0u, 1u, 2u, 3u}) {
            runtime = Runtime{};
            const auto result = quick
                ? pal4::QuickDeadSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime)
                : pal4::DeadSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime);
            Check(result == kTask &&
                      (std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "free_task") != runtime.calls.end()) ==
                          ((flags & 1u) != 0u),
                  "Dead deleting destructor honors low flag bit");
            Check(Index(runtime, "cleanup_collection") <
                      Index(runtime, "destroy_container") &&
                      runtime.calls.back() == ((flags & 1u) != 0u
                          ? "free_task" : "destroy_container"),
                  "Dead deletion follows complete base cleanup");
            if ((flags & 1u) == 0u) VerifyCommon(runtime);
        }
    }
}

void WoundedDestructors() {
    for (const auto kind : {0u, 1u, 2u}) {
        Runtime runtime;
        const auto base_result = kind == 0u
            ? pal4::QuickWoundedSequenceTask::DestroyBase(kTask, runtime)
            : kind == 1u
                ? pal4::WoundedSequenceTask::DestroyBase(kTask, runtime)
                : pal4::FinalHitSequenceTask::DestroyBase(kTask, runtime);
        Check(base_result == runtime.destroy_result,
              "Wounded base destructor forwards GOM result");
        VerifyCommon(runtime);

        for (const auto flags : {0u, 1u, 2u, 3u}) {
            runtime = Runtime{};
            const auto result = kind == 0u
                ? pal4::QuickWoundedSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime)
                : kind == 1u
                    ? pal4::WoundedSequenceTask::Destroy(
                          kTask, static_cast<std::uint8_t>(flags), runtime)
                    : pal4::FinalHitSequenceTask::Destroy(
                          kTask, static_cast<std::uint8_t>(flags), runtime);
            Check(result == kTask &&
                      (std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "free_task") != runtime.calls.end()) ==
                          ((flags & 1u) != 0u),
                  "Wounded deleting destructor honors low flag bit");
            Check(Index(runtime, "cleanup_collection") <
                      Index(runtime, "destroy_container") &&
                      runtime.calls.back() == ((flags & 1u) != 0u
                          ? "free_task" : "destroy_container"),
                  "Wounded deletion follows complete base cleanup");
            if ((flags & 1u) == 0u) VerifyCommon(runtime);
        }
    }
}

void FendOffDestructors() {
    for (bool quick : {false, true}) {
        Runtime runtime;
        const auto base_result = quick
            ? pal4::QuickFendOffSequenceTask::DestroyBase(kTask, runtime)
            : pal4::FendOffSequenceTask::DestroyBase(kTask, runtime);
        Check(base_result == runtime.destroy_result,
              "FendOff base destructor forwards GOM result");
        VerifyCommon(runtime);

        for (const auto flags : {0u, 1u, 2u, 3u}) {
            runtime = Runtime{};
            const auto result = quick
                ? pal4::QuickFendOffSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime)
                : pal4::FendOffSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime);
            Check(result == kTask &&
                      (std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "free_task") != runtime.calls.end()) ==
                          ((flags & 1u) != 0u),
                  "FendOff deleting destructor honors low flag bit");
            Check(Index(runtime, "cleanup_collection") <
                      Index(runtime, "destroy_container") &&
                      runtime.calls.back() == ((flags & 1u) != 0u
                          ? "free_task" : "destroy_container"),
                  "FendOff deletion follows complete base cleanup");
            if ((flags & 1u) == 0u) VerifyCommon(runtime);
        }
    }
}

void MissDestructorsAndTarget() {
    for (bool quick : {false, true}) {
        Runtime runtime;
        const auto base_result = quick
            ? pal4::QuickMissSequenceTask::DestroyBase(kTask, runtime)
            : pal4::MissSequenceTask::DestroyBase(kTask, runtime);
        Check(base_result == runtime.destroy_result,
              "Miss base destructor forwards GOM result");
        VerifyCommon(runtime);

        for (const auto flags : {0u, 1u, 2u, 3u}) {
            runtime = Runtime{};
            const auto result = quick
                ? pal4::QuickMissSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime)
                : pal4::MissSequenceTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime);
            Check(result == kTask &&
                      (std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "free_task") != runtime.calls.end()) ==
                          ((flags & 1u) != 0u),
                  "Miss deleting destructor honors low flag bit");
            Check(Index(runtime, "cleanup_collection") <
                      Index(runtime, "destroy_container") &&
                      runtime.calls.back() == ((flags & 1u) != 0u
                          ? "free_task" : "destroy_container"),
                  "Miss deletion follows complete base cleanup");
            if ((flags & 1u) == 0u) VerifyCommon(runtime);
        }
    }

    Runtime runtime;
    runtime.words[kTask + 48u] = 0x10203040u;
    Check(pal4::ScriptTaskBase::GetTarget(kTask, runtime) == 0x10203040u &&
              runtime.calls == std::vector<std::string>{"read:12336"},
          "shared target getter reads task word +48 only");
}

void EndWinAndLevelUpActionDestructors() {
    for (int kind : {0, 1, 2}) {
        Runtime runtime;
        const auto result = kind == 0
            ? pal4::CombatEndActionTask::DestroyBase(kTask, runtime)
            : kind == 1
                ? pal4::WinActionTask::DestroyBase(kTask, runtime)
                : pal4::LevelUpActionTask::DestroyBase(kTask, runtime);
        Check(result == runtime.destroy_result,
              "End/Win/LevelUp base destructor forwards container result");
        VerifyCommon(runtime);

        for (const auto flags : {0u, 1u, 2u, 3u}) {
            runtime = Runtime{};
            const auto task = kind == 0
                ? pal4::CombatEndActionTask::Destroy(
                      kTask, static_cast<std::uint8_t>(flags), runtime)
                : kind == 1
                    ? pal4::WinActionTask::Destroy(
                          kTask, static_cast<std::uint8_t>(flags), runtime)
                    : pal4::LevelUpActionTask::Destroy(
                          kTask, static_cast<std::uint8_t>(flags), runtime);
            Check(task == kTask &&
                      (std::find(runtime.calls.begin(), runtime.calls.end(),
                                 "free_task") != runtime.calls.end()) ==
                          ((flags & 1u) != 0u),
                  "End/Win/LevelUp deleting destructor honors low flag bit");
            Check(Index(runtime, "cleanup_collection") <
                      Index(runtime, "destroy_container") &&
                      runtime.calls.back() == ((flags & 1u) != 0u
                          ? "free_task" : "destroy_container"),
                  "End/Win/LevelUp deletion follows complete base cleanup");
            if ((flags & 1u) == 0u) VerifyCommon(runtime);
        }
    }
}

void EndStuntDestructorAndActorGetter() {
    Runtime runtime;
    runtime.words[kTask + 44u] = 0x10203040u;
    Check(pal4::ScriptTaskBase::GetActor(kTask, runtime) ==
              0x10203040u &&
              runtime.calls == std::vector<std::string>{"read:12332"},
          "shared actor getter reads only task word +44");

    runtime = Runtime{};
    Check(pal4::EndStuntSequenceTask::DestroyBase(kTask, runtime) ==
              runtime.destroy_result,
          "EndStunt base destructor forwards container result");
    VerifyCommon(runtime);
    for (const auto flags : {0u, 1u, 2u, 3u}) {
        runtime = Runtime{};
        Check(pal4::EndStuntSequenceTask::Destroy(
                  kTask, static_cast<std::uint8_t>(flags), runtime) ==
                  kTask &&
                  (std::find(runtime.calls.begin(), runtime.calls.end(),
                             "free_task") != runtime.calls.end()) ==
                      ((flags & 1u) != 0u),
              "EndStunt deleting destructor honors low flag bit");
        Check(Index(runtime, "cleanup_collection") <
                  Index(runtime, "destroy_container"),
              "EndStunt deletes after base cleanup");
    }
}

void ActorStatusGetter() {
    struct StatusRuntime final : pal4::ICombatTaskActorStatusRuntime {
        std::vector<std::string> calls;
        std::uint32_t ReadWord(std::uint32_t address) override {
            Check(address == kTask + 44u,
                  "actor status getter reads source field");
            calls.emplace_back("read_actor");
            return 0x1234u;
        }
        std::uint32_t ActorStatusVirtual480(
            std::uint32_t actor) override {
            Check(actor == 0x1234u,
                  "actor status virtual receives source");
            calls.emplace_back("virtual480");
            return 0xdeadbeefu;
        }
    } runtime;
    Check(pal4::ScriptTaskBase::GetActorStatus(kTask, runtime) ==
              0xdeadbeefu &&
              runtime.calls == std::vector<std::string>{
                  "read_actor", "virtual480"},
          "shared actor status forwards virtual return");
}

}  // namespace

int main() {
    ReleaseOwnedText();
    DecrementSharedText();
    EmptyText();
    StartAttackBaseDestructor();
    PhysicalAttackBaseDestructor();
    EndAttackBaseDestructor();
    QuickAttackBaseDestructor();
    QuickAttackEndDestructors();
    CastMagicDestructors();
    StuntDestructors();
    UsePropertyDestructors();
    DefendDestructors();
    FleeDestructorsAndZeroCallback();
    DeadDestructors();
    WoundedDestructors();
    FendOffDestructors();
    MissDestructorsAndTarget();
    EndWinAndLevelUpActionDestructors();
    EndStuntDestructorAndActorGetter();
    ActorStatusGetter();
    std::cout << "Script task base tests passed\n";
}
