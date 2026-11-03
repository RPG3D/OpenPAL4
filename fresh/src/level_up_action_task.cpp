#include "pal4/level_up_action_task.h"

#include "pal4/win_action_task.h"
#include "pal4/script_task_base.h"

#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::uint32_t kTextHeader = 0x8fa2b8u;
constexpr std::uint32_t kEmptyText = 0x841964u;
constexpr std::array<std::uint32_t, 3> kTiming{1u, 0u, 0u};

std::uint32_t TextOrEmpty(ILevelUpActionRuntime& runtime) {
    const auto data = runtime.ReadWord(kTextHeader + 4u);
    return data == 0u ? kEmptyText : data;
}

std::uint8_t AssignLegacyLevelUpText(const std::uint32_t child,
                               ILevelUpActionRuntime& runtime) {
    const auto destination = child + 64u;
    const auto length = runtime.ReadWord(kTextHeader + 8u);
    if (destination == kTextHeader) {
        if (runtime.ReadWord(destination + 8u) < length)
            throw std::out_of_range("legacy level-up text alias length");
        runtime.DetachText(destination);
        const auto remaining = runtime.ReadWord(destination + 8u) - length;
        if (remaining != 0u) {
            const auto erase_count = remaining == 0xffffffffu
                                         ? 0xffffffffu : remaining;
            const auto data = runtime.ReadWord(destination + 4u);
            runtime.MoveTextBytes(data + length,
                                  data + length + erase_count,
                                  remaining - erase_count);
            const auto new_length =
                runtime.ReadWord(destination + 8u) - erase_count;
            if (runtime.AllocateText(destination, new_length, 0u))
                runtime.SetTextSize(destination, new_length);
        }
        return runtime.DetachTextAndReturn(destination);
    }

    if (length != 0u) {
        const auto data = TextOrEmpty(runtime);
        const auto references = runtime.ReadByte(data - 1u);
        if (references < 0xfeu) {
            const auto old_data = runtime.ReadWord(destination + 4u);
            if (old_data != 0u) {
                const auto old_references = runtime.ReadByte(old_data - 1u);
                if (old_references == 0u || old_references == 0xffu)
                    runtime.FreeText(old_data - 1u);
                else
                    runtime.WriteByte(old_data - 1u,
                                      static_cast<std::uint8_t>(
                                          old_references - 1u));
            }
            runtime.WriteWord(destination + 4u, 0u);
            runtime.WriteWord(destination + 8u, 0u);
            runtime.WriteWord(destination + 12u, 0u);
            const auto shared = TextOrEmpty(runtime);
            runtime.WriteWord(destination + 4u, shared);
            runtime.WriteWord(destination + 8u,
                              runtime.ReadWord(kTextHeader + 8u));
            runtime.WriteWord(destination + 12u,
                              runtime.ReadWord(kTextHeader + 12u));
            runtime.WriteByte(shared - 1u,
                              static_cast<std::uint8_t>(references + 1u));
            return static_cast<std::uint8_t>(shared);
        }
    }

    if (!runtime.AllocateText(destination, length, 1u)) return 0u;
    runtime.CopyText(destination, TextOrEmpty(runtime), length);
    runtime.WriteWord(destination + 8u, length);
    runtime.WriteByte(runtime.ReadWord(destination + 4u) + length, 0u);
    return static_cast<std::uint8_t>(length);
}

}  // namespace

std::uint8_t ILevelUpActionRuntime::AssignLevelUpText(std::uint32_t child){return AssignLegacyLevelUpText(child,*this);}

std::uint8_t LevelUpActionTask::Start(const std::uint32_t task,
                                      ILevelUpActionRuntime& runtime) {
    runtime.SetActorState(runtime.ReadWord(task + 44u), 7u, 1u);
    const auto configured = runtime.ReadWord(0x8f95d0u);
    const auto text = configured == 0u ? kEmptyText : configured;
    WinActionChildFactorySlot factory;
    (void)WinActionThirdChildFactory::Initialize(&factory);
    const auto child = runtime.CreateChildTask(factory.vtable);
    if (child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, child, text, kTiming);
    runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
    return runtime.AssignLevelUpText(child);
}

std::uint32_t LevelUpActionTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t LevelUpActionTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}


}  // namespace pal4
