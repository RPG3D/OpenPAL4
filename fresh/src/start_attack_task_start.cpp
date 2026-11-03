#include "pal4/start_attack_task.h"

namespace pal4 {

std::array<float, 3>* ScriptVector3Operations::Add(
    std::array<float, 3>& destination,
    std::array<float, 3>& operand) {
    for (std::uint32_t i = 0u; i < 3u; ++i)
        destination[i] += operand[i];
    return &operand;
}

std::array<float, 3>* ScriptVector3Operations::Subtract(
    std::array<float, 3>& destination,
    std::array<float, 3>& operand) {
    for (std::uint32_t i = 0u; i < 3u; ++i)
        destination[i] -= operand[i];
    return &operand;
}

StartAttackApproachFactorySlot* StartAttackApproachFactory::InitializePrimary(
    StartAttackApproachFactorySlot* const slot) {
    slot->vtable = kPrimaryVtable;
    return slot;
}

StartAttackApproachFactorySlot* StartAttackApproachFactory::InitializeSecondary(
    StartAttackApproachFactorySlot* const slot) {
    slot->vtable = kSecondaryVtable;
    return slot;
}

std::uint32_t StartAttackTask::Start(
    const std::uint32_t task, IStartAttackExecutionRuntime& runtime) {
    const auto source = runtime.ReadWord(task + 44u);
    const auto active = runtime.ReadWord(source + 808u);
    if (active == 0u) return source;

    const auto protector = runtime.ReadWord(task + 48u);
    auto protector_position = runtime.ActorChildPosition(protector);
    auto direction = runtime.ActorChildPosition(source);
    (void)ScriptVector3Operations::Subtract(direction, protector_position);
    runtime.NormalizeVector(direction);
    const auto distance = runtime.ActorClampedChildSize(protector);
    for (auto& component : direction) component *= distance;
    const auto protector_state = runtime.ReadWord(protector + 804u);
    const auto source_state = runtime.ReadWord(source + 804u);
    if (source_state != protector_state)
        (void)ScriptVector3Operations::Add(protector_position, direction);

    runtime.NotifySource(source);
    const auto blocked = runtime.SourceBlocksEvent(source);
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    const auto first_text = runtime.GlobalText(0x8f9784u);
    StartAttackApproachFactorySlot factory;
    (void)StartAttackApproachFactory::InitializePrimary(&factory);
    const auto first = runtime.CreateApproachTask(factory);
    if (first == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, first, first_text, timing);
    const auto first_position = blocked
        ? runtime.ActorTargetPosition(protector) : protector_position;
    (void)runtime.SetApproachTaskTarget(first, source, first_position);

    const auto completion = runtime.SourceEndsSequence(source);
    if (completion != 0u) return completion;
    const auto current_protector_state = runtime.ReadWord(protector + 804u);
    const auto current_source_state = runtime.ReadWord(source + 804u);
    if (current_source_state == current_protector_state)
        return current_protector_state;

    const auto second_text = runtime.GlobalText(0x8f97acu);
    (void)StartAttackApproachFactory::InitializeSecondary(&factory);
    const auto second = runtime.CreateApproachTask(factory);
    if (second == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, second, second_text, timing);
    const auto second_position = blocked
        ? runtime.ActorTargetPosition(protector) : protector_position;
    return runtime.SetApproachTaskTarget(second, source, second_position);
}

}  // namespace pal4
