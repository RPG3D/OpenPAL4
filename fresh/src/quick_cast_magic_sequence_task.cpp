#include "pal4/quick_cast_magic_sequence_task.h"
#include "pal4/script_task_base.h"

namespace pal4 {

std::uint32_t QuickCastMagicSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t QuickCastMagicSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

QuickCastMagicChildFactorySlot* QuickCastMagicChildFactory::Initialize(
    QuickCastMagicChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

std::uint32_t QuickCastMagicSequenceTask::Start(
    const std::uint32_t task, IQuickCastMagicSequenceRuntime& runtime) {
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    runtime.NotifySource(runtime.ReadWord(task + 44u));
    const auto configured_text = runtime.ReadWord(0x8f9720u);
    const auto text = configured_text == 0u ? 0x841964u : configured_text;
    QuickCastMagicChildFactorySlot factory;
    (void)QuickCastMagicChildFactory::Initialize(&factory);
    const auto child = runtime.CreateChildTask(factory.vtable);
    if (child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, child, text, timing);
    const auto magic = runtime.ReadWord(task + 52u);
    const auto source = runtime.ReadWord(task + 44u);
    const auto target = runtime.ReadWord(task + 48u);
    runtime.WriteWord(child + 64u, target);
    runtime.WriteWord(child + 60u, source);
    runtime.WriteWord(child + 68u, magic);
    return magic;
}

}  // namespace pal4
