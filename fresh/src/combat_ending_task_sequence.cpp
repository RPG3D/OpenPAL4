#include "pal4/combat_ending_task_sequence.h"
#include "pal4/combat_reward_record_layout.h"

#include <bit>

namespace pal4 {
namespace {

class ActorIterator final {
public:
    ActorIterator(ICombatEndingTaskSequenceRuntime& runtime,
                  std::uint32_t handle)
        : runtime_(runtime), handle_(handle) {}
    ~ActorIterator() {
        if (handle_ != 0u) (void)runtime_.ReleaseActors(handle_);
    }
    ActorIterator(const ActorIterator&) = delete;
    ActorIterator& operator=(const ActorIterator&) = delete;
    [[nodiscard]] std::uint32_t Handle() const { return handle_; }
    [[nodiscard]] int Release() {
        if (handle_ == 0u) return 0;
        const auto handle = handle_;
        handle_ = 0u;
        return runtime_.ReleaseActors(handle);
    }

private:
    ICombatEndingTaskSequenceRuntime& runtime_;
    std::uint32_t handle_;
};

void WriteFloat(ICombatEndingTaskSequenceRuntime& runtime,
                std::uint32_t address, float value) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value));
}

[[nodiscard]] std::uint32_t AddTask(
    ICombatEndingTaskSequenceRuntime& runtime,
    std::uint32_t container, CombatEndingTaskType type,
    std::uint32_t name_address, float time) {
    const auto name = runtime.GlobalName(name_address);
    const auto task = runtime.CreateTask(type);
    if (task != 0u)
        runtime.AddTask(container + 4u, task, name, 2, time, 0);
    return task;
}

void SetWindowTask(ICombatEndingTaskSequenceRuntime& runtime,
                   std::uint32_t task, std::uint32_t window,
                   std::uint8_t visible, std::uint8_t property) {
    runtime.WriteWord(task + 60u, window);
    runtime.WriteByte(task + 64u, visible);
    runtime.WriteByte(task + 65u, property);
}

void ConfigureExperienceTask(ICombatEndingTaskSequenceRuntime& runtime,
                             std::uint32_t dialog,
                             std::uint32_t task,
                             std::uint32_t actor,
                             std::uint32_t slot,
                             std::uint32_t row_index,
                             std::uint32_t row) {
    runtime.WriteWord(task + 60u, row);
    runtime.WriteWord(task + 64u, dialog + 420u);
    runtime.WriteWord(task + 68u, actor);
    runtime.WriteWord(task + 92u, dialog + 700u + 4u * row_index);
    runtime.WriteWord(task + 96u,
                      runtime.ReadWord(dialog + 684u + 4u * row_index));

    const auto level = runtime.ActorStat(actor, 4u);
    const auto current_exp = runtime.ActorStat(actor, 5u);
    const auto required = runtime.RequiredExperience(
        slot + 1001u, level + 1);
    runtime.WriteWord(task + 88u,
                      static_cast<std::uint32_t>(required - current_exp));
    const auto record = dialog + 472u + 40u * slot;
    const auto start = static_cast<std::int32_t>(
        runtime.ReadWord(record + CombatRewardRecordLayout::kPreviousExperience));
    const auto finish = static_cast<std::int32_t>(
        runtime.ReadWord(record + CombatRewardRecordLayout::kUpdatedExperience));
    WriteFloat(runtime, task + 72u, static_cast<float>(start));
    WriteFloat(runtime, task + 76u,
               static_cast<float>(finish - start) * 0.5f);
    runtime.WriteWord(task + 80u, 0u);
    WriteFloat(runtime, task + 84u, static_cast<float>(start));
    const auto id = runtime.ActorRoleKey(actor);
    if (const auto attribute = runtime.FindAttribute(id, start)) {
        runtime.WriteWord(task + 80u, attribute->image);
        WriteFloat(runtime, task + 84u,
                   static_cast<float>(attribute->limit - start - 1));
    }
}

}  // namespace

std::uint32_t ICombatEndingTaskSequenceRuntime::CreateSequence() {
    const auto container = Allocate(0x2cu);
    if (container != 0u) {
        InitializeContainer(container + 4u);
        InitializeName(container + 20u, "EndTask");
        WriteWord(container + 36u, 0u);
        WriteWord(container, CombatEndingTaskSequence::kContainerVtable);
        WriteByte(container + 40u, 1u);
    }
    return container;
}

int CombatEndingTaskSequence::Start(
    const std::uint32_t dialog,
    ICombatEndingTaskSequenceRuntime& runtime) {
    runtime.WriteWord(dialog + 716u, 0u);
    const auto container = runtime.CreateSequence();

    float time = 0.0f;
    if (!runtime.CombatInactive()) {
        auto task = AddTask(runtime, container,
                            CombatEndingTaskType::ScoreText,
                            0x8f94f4u, 1.0f);
        runtime.WriteWord(task + 60u, dialog);
        task = AddTask(runtime, container,
                       CombatEndingTaskType::LevelText,
                       0x8f94e0u, 2.0f);
        runtime.WriteWord(task + 60u, dialog);
        time = 4.0f;
    }

    auto task = AddTask(runtime, container, CombatEndingTaskType::Window,
                        0x8f956cu, time);
    SetWindowTask(runtime, task, runtime.ReadWord(dialog + 80u), 1u, 0u);
    task = AddTask(runtime, container, CombatEndingTaskType::Window,
                   0x8f956cu, time);
    SetWindowTask(runtime, task, runtime.ReadWord(dialog + 84u), 1u, 0u);

    ActorIterator actors(runtime, runtime.BeginActors(37u));
    auto row = dialog + 116u;
    std::uint32_t row_index = 0u;
    while (!runtime.ActorsDone(actors.Handle())) {
        const auto actor = runtime.CurrentActor(actors.Handle());
        const auto slot = runtime.ReadWord(actor + 0x4a8u);
        if (slot < 4u) {
            time += 0.5f;
            task = AddTask(runtime, container,
                           CombatEndingTaskType::MoveDownWindow,
                           0x8f9530u, time);
            runtime.WriteWord(task + 60u, runtime.ReadWord(row));
            runtime.WriteWord(task + 64u, runtime.ReadWord(row + 60u));
            if (runtime.ActorStat(actor, 4u) < 99) {
                task = AddTask(runtime, container,
                               CombatEndingTaskType::UpdateExperience,
                               0x8f9544u, time);
                ConfigureExperienceTask(runtime, dialog, task, actor, slot,
                                        row_index, row);
            }
            row += 76u;
            ++row_index;
        }
        runtime.AdvanceActor(actors.Handle());
    }

    task = AddTask(runtime, container, CombatEndingTaskType::ShowStep,
                   0x8f9580u, time);
    runtime.WriteWord(task + 60u, dialog);
    runtime.WriteWord(task + 64u, 1u);
    time += 1.0f;
    task = AddTask(runtime, container, CombatEndingTaskType::Window,
                   0x8f956cu, time);
    SetWindowTask(runtime, task, runtime.ReadWord(dialog + 72u), 1u, 0u);
    const auto property_count = runtime.ReadWord(dialog + 452u);
    if (property_count != 0u) {
        task = AddTask(runtime, container, CombatEndingTaskType::Window,
                       0x8f956cu, time);
        SetWindowTask(runtime, task, runtime.ReadWord(dialog + 76u), 1u, 1u);
        time += 1.5f + static_cast<float>(property_count) * 0.5f;
        task = AddTask(runtime, container, CombatEndingTaskType::Window,
                       0x8f956cu, time);
        SetWindowTask(runtime, task, runtime.ReadWord(dialog + 76u), 0u, 0u);
    } else {
        time += 2.0f;
    }
    task = AddTask(runtime, container, CombatEndingTaskType::Window,
                   0x8f956cu, time);
    SetWindowTask(runtime, task, runtime.ReadWord(dialog + 72u), 0u, 0u);
    time += 0.5f;
    task = AddTask(runtime, container, CombatEndingTaskType::ShowStep,
                   0x8f9580u, time);
    runtime.WriteWord(task + 60u, dialog);
    runtime.WriteWord(task + 64u, 2u);
    runtime.SetVisible(runtime.ReadWord(dialog + 88u), false);

    const auto prize_count = runtime.ReadWord(dialog + 468u);
    if (prize_count != 0u) {
        task = AddTask(runtime, container,
                       CombatEndingTaskType::PrizeProperty,
                       0x8f9508u, time);
        runtime.WriteWord(task + 60u, dialog);
        time += static_cast<float>(prize_count) * 0.5f;
    }
    task = AddTask(runtime, container, CombatEndingTaskType::CloseSystem,
                   0x8f9558u, time + 2.0f);
    runtime.WriteWord(task + 60u, runtime.ReadWord(dialog + 436u));

    runtime.ResetFlags(dialog + 44u);
    runtime.RegisterSequence(dialog + 44u, container);
    runtime.ResetActorManagerFlags();
    ActorIterator candidates(runtime, runtime.BeginActors(38u));
    while (!runtime.ActorsDone(candidates.Handle())) {
        const auto actor = runtime.CurrentActor(candidates.Handle());
        if (runtime.ActorStateNotThree(actor)) {
            runtime.ResetObjectProperties(actor);
            runtime.RegisterActorEvent(runtime.CombatWinAction(actor));
        }
        runtime.AdvanceActor(candidates.Handle());
    }
    const auto had_actors = actors.Handle() != 0u;
    const auto candidate_result = candidates.Release();
    const auto actor_result = actors.Release();
    return had_actors ? actor_result : candidate_result;
}

}  // namespace pal4
