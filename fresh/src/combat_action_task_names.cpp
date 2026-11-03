#include "pal4/combat_action_task_names.h"

namespace pal4 {
namespace {

const CombatActionTaskNameBinding& Binding(const CombatActionTaskName task) {
    return CombatActionTaskNames::kBindings[static_cast<std::uint8_t>(task)];
}

}  // namespace

std::uint8_t CombatActionTaskNames::Create(
    const CombatActionTaskName task, const std::uint8_t incoming_header,
    ICombatActionTaskNameRuntime& runtime) {
    const auto& binding = Binding(task);
    runtime.WriteByte(binding.buffer, incoming_header);
    runtime.WriteWord(binding.buffer + 4u, 0u);
    runtime.WriteWord(binding.buffer + 8u, 0u);
    runtime.WriteWord(binding.buffer + 12u, 0u);
    runtime.SetErrorFlag(binding.buffer, 1u);
    if (binding.text.empty())
        return runtime.ReadByte(binding.buffer);
    const auto length = static_cast<std::uint32_t>(binding.text.size());
    const auto destination = runtime.ReallocateString(binding.buffer, length);
    runtime.CopyBytes(destination, binding.text);
    runtime.WriteWord(binding.buffer + 8u, length);
    runtime.WriteByte(destination + length, 0u);
    return static_cast<std::uint8_t>(destination);
}

int CombatActionTaskNames::InitializeAndRegister(
    const CombatActionTaskName task, const std::uint8_t incoming_header,
    ICombatActionTaskNameRuntime& runtime) {
    (void)Create(task, incoming_header, runtime);
    return runtime.RegisterAtExit(Binding(task).destructor);
}

void CombatActionTaskNames::Cleanup(
    const CombatActionTaskName task,
    ICombatActionTaskNameRuntime& runtime) {
    const auto& binding = Binding(task);
    const auto guard = runtime.ReadByte(binding.guard);
    if ((guard & binding.guard_mask) != 0u) return;
    runtime.WriteByte(binding.guard,
                      static_cast<std::uint8_t>(guard | binding.guard_mask));
    const auto data = runtime.ReadWord(binding.buffer + 4u);
    if (data) {
        const auto refs = runtime.ReadByte(data - 1u);
        if (refs == 0u || refs == 0xffu)
            runtime.Free(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(refs - 1u));
    }
    runtime.WriteWord(binding.buffer + 4u, 0u);
    runtime.WriteWord(binding.buffer + 8u, 0u);
    runtime.WriteWord(binding.buffer + 12u, 0u);
}

}  // namespace pal4
