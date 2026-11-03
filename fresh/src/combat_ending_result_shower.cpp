#include "pal4/combat_ending_result_shower.h"
#include "pal4/combat_reward_record_layout.h"

#include <bit>
#include <string>

namespace pal4 {
namespace {

class ActorIterator final {
public:
    ActorIterator(ICombatEndingResultShowRuntime& runtime,
                  std::uint32_t handle)
        : runtime_(runtime), handle_(handle) {}
    ~ActorIterator() {
        if (handle_ != 0u) (void)runtime_.ReleaseActors(handle_);
    }
    ActorIterator(const ActorIterator&) = delete;
    ActorIterator& operator=(const ActorIterator&) = delete;
    [[nodiscard]] std::uint32_t Handle() const { return handle_; }
    [[nodiscard]] int Release() {
        const auto handle = handle_;
        handle_ = 0u;
        return handle == 0u ? 0 : runtime_.ReleaseActors(handle);
    }

private:
    ICombatEndingResultShowRuntime& runtime_;
    std::uint32_t handle_;
};

[[nodiscard]] std::uint32_t AddTask(
    ICombatEndingResultShowRuntime& runtime,
    std::uint32_t container, CombatEndingTaskType type,
    std::uint32_t name_address, float time) {
    const auto name = runtime.GlobalName(name_address);
    const auto task = runtime.CreateTask(type);
    if (task != 0u)
        runtime.AddTask(container + 4u, task, name, 2, time, 0);
    return task;
}

void SetWindowTask(ICombatEndingResultShowRuntime& runtime,
                   std::uint32_t task, std::uint32_t window,
                   std::uint8_t visible, std::uint8_t property) {
    runtime.WriteWord(task + 60u, window);
    runtime.WriteByte(task + 64u, visible);
    runtime.WriteByte(task + 65u, property);
}

}  // namespace

std::uint32_t ICombatEndingResultShowRuntime::CreateSequence() {
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

int CombatEndingResultShower::Show(
    const std::uint32_t dialog,
    ICombatEndingResultShowRuntime& runtime) {
    runtime.CleanupCombatConsole();
    runtime.WriteWord(dialog + 716u, 1u);
    const auto container = runtime.CreateSequence();
    runtime.SetVisible(runtime.ReadWord(dialog + 80u), true);
    runtime.SetVisible(runtime.ReadWord(dialog + 84u), true);

    ActorIterator actors(runtime, runtime.BeginActors());
    auto row = dialog + 116u;
    std::uint32_t row_index = 0u;
    while (!runtime.ActorsDone(actors.Handle())) {
        const auto actor = runtime.CurrentActor(actors.Handle());
        const auto slot = runtime.ReadWord(actor + 0x4a8u);
        if (slot < 4u) {
            const auto record = dialog + 472u + 40u * slot;
            runtime.SetYPosition(runtime.ReadWord(row),
                std::bit_cast<float>(runtime.ReadWord(row + 60u)));
            if (runtime.ActorStat(actor, 4u) < 99) {
                const auto exp = runtime.ActorStat(actor, 5u);
                const auto starting_exp = static_cast<std::int32_t>(
                    runtime.ReadWord(record + CombatRewardRecordLayout::kPreviousExperience));
                runtime.SetText(runtime.ReadWord(row + 20u),
                                std::to_string(exp - starting_exp));
                const auto level = runtime.ActorStat(actor, 4u);
                const auto required = runtime.RequiredExperience(
                    slot + 1001u, level + 1);
                runtime.SetText(runtime.ReadWord(row + 24u),
                                std::to_string(required -
                                    runtime.ActorStat(actor, 5u)));
            }
            runtime.HandleLevelUp(
                row, runtime.ActorStat(actor, 4u), actor,
                runtime.ReadWord(dialog + 684u + 4u * row_index), record);
            row += 76u;
            ++row_index;
        }
        runtime.AdvanceActor(actors.Handle());
    }

    auto task = AddTask(runtime, container, CombatEndingTaskType::Window,
                        0x8f956cu, 0.0f);
    SetWindowTask(runtime, task, runtime.ReadWord(dialog + 72u), 1u, 0u);
    float time;
    const auto property_count = runtime.ReadWord(dialog + 452u);
    if (property_count != 0u) {
        task = AddTask(runtime, container, CombatEndingTaskType::Window,
                       0x8f956cu, 0.0f);
        SetWindowTask(runtime, task, runtime.ReadWord(dialog + 76u), 1u, 1u);
        time = 1.5f + static_cast<float>(property_count) * 0.5f;
        task = AddTask(runtime, container, CombatEndingTaskType::Window,
                       0x8f956cu, time);
        SetWindowTask(runtime, task, runtime.ReadWord(dialog + 76u), 0u, 0u);
    } else {
        time = 2.0f;
    }
    task = AddTask(runtime, container, CombatEndingTaskType::Window,
                   0x8f956cu, time);
    SetWindowTask(runtime, task, runtime.ReadWord(dialog + 72u), 0u, 0u);
    time += 0.5f;
    task = AddTask(runtime, container, CombatEndingTaskType::ShowStep,
                   0x8f9580u, time);
    runtime.WriteWord(task + 60u, dialog);
    runtime.WriteWord(task + 64u, 2u);
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
    const auto result = runtime.RegisterSequence(dialog + 44u, container);
    const auto had_actors = actors.Handle() != 0u;
    const auto release = actors.Release();
    return had_actors ? release : result;
}

int CombatEndingResultShower::Hide(
    const std::uint32_t dialog,
    ICombatEndingResultShowRuntime& runtime) {
    runtime.CleanupCombatConsole();
    runtime.WriteWord(dialog + 716u, 2u);
    const auto container = runtime.CreateSequence();
    runtime.SetVisible(runtime.ReadWord(dialog + 72u), false);
    runtime.SetVisible(runtime.ReadWord(dialog + 76u), false);
    const auto prize_count = runtime.ReadWord(dialog + 468u);
    if (prize_count != 0u) {
        auto task = AddTask(runtime, container,
                            CombatEndingTaskType::PrizeProperty,
                            0x8f9508u, 0.0f);
        runtime.WriteWord(task + 60u, dialog);
    }
    const auto close_time = 2.0f +
        static_cast<float>(prize_count) * 0.5f;
    const auto task = AddTask(runtime, container,
                              CombatEndingTaskType::CloseSystem,
                              0x8f9558u, close_time);
    runtime.WriteWord(task + 60u, runtime.ReadWord(dialog + 436u));
    runtime.ResetFlags(dialog + 44u);
    return runtime.RegisterSequence(dialog + 44u, container);
}

}  // namespace pal4
