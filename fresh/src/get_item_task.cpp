#include "pal4/get_item_task.h"

#include "pal4/action_task_object.h"
#include "pal4/task_pointer_collection.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t GetItemTask::Construct(
    const std::uint32_t object, IGetItemTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    for (const auto offset : {0x40u, 0x44u, 0x48u})
        runtime.WriteWord(object + offset, 0u);
    return object;
}

std::uint32_t GetItemTask::Destruct(
    const std::uint32_t object, IGetItemTaskRuntime& runtime) {
    runtime.WriteWord(object, 0x8486fcu);
    runtime.WriteByte(object + 0x3cu, 0u);
    for (const auto offset : {0x40u, 0x44u, 0x48u})
        runtime.WriteWord(object + offset, 0u);
    return runtime.BaseDestruct(object);
}

std::uint32_t GetItemTask::Delete(
    const std::uint32_t object, const std::uint8_t flags,
    IGetItemTaskRuntime& runtime) {
    (void)Destruct(object, runtime);
    if ((flags & 1u) != 0u) runtime.FreeMemory(object);
    return object;
}

std::uint8_t GetItemTask::Enter(
    const std::uint32_t object, IGetItemTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x4cu) == 0u) return 0u;
    const auto formatted = runtime.BeginText();
    std::uint32_t hint = 0u;
    bool hint_initialized = false;
    try {
        hint = runtime.BeginText();
        hint_initialized = true;
        const auto kind = runtime.ReadWord(object + 0x40u);
        const auto id = runtime.ReadWord(object + 0x44u);
        const auto amount = runtime.ReadWord(object + 0x48u);
        if (kind == 0u && runtime.HasItem(id)) {
            const auto name = runtime.ItemName(id);
            runtime.GrantItem(id, amount);
            runtime.FormatText(formatted, runtime.GameString(0xfbu),
                               name, amount);
            runtime.AppendText(hint, formatted);
            runtime.PlaySound("gamedata\\PALSound\\WA06.wav");
        } else if (kind == 1u && runtime.HasEquipment(id)) {
            const auto name = runtime.EquipmentName(id);
            runtime.GrantEquipment(id);
            runtime.FormatText(formatted, runtime.GameString(0xfbu),
                               name, 1u);
            runtime.AppendText(hint, formatted);
            runtime.PlaySound("gamedata\\PALSound\\WA06.wav");
        } else if (kind == 2u) {
            runtime.AddMoney(amount);
            runtime.FormatText(formatted, runtime.GameString(0xc2u),
                               {}, amount);
            runtime.AppendText(hint, formatted);
            runtime.PlaySound("gamedata\\PALSound\\WA40.wav");
        }
        const auto window = runtime.CreateHintWindow();
        if (window != 0u)
            runtime.ShowHint(window, hint, 0.5f, 0.15f,
                             1.0f, false, true);
    } catch (...) {
        if (hint_initialized) runtime.ReleaseText(hint);
        runtime.ReleaseText(formatted);
        throw;
    }
    runtime.ReleaseText(hint);
    runtime.ReleaseText(formatted);
    return 1u;
}

std::uint8_t GetItemTask::Update(
    const std::uint32_t object, IGetItemTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x4cu) == 0u) {
        runtime.WriteByte(object + 0x3cu, 0u);
        return 0u;
    }
    if (runtime.ReadByte(object + 0x3cu) != 0u) return 0u;
    const auto window = runtime.CreateHintWindow();
    if (window != 0u && !runtime.HintVisible(window))
        runtime.WriteByte(object + 0x3cu, 1u);
    return 1u;
}

std::uint8_t GetItemTask::SpawnFollowup(
    const std::uint32_t object, IGetItemTaskSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x50u);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x4cu);
    if (owner == 0u) return 0u;
    const auto previous = runtime.ReadWord(object + 0x30u);
    if (previous != 0u) {
        runtime.NotifyPreviousTask(previous);
        return 1u;
    }
    const auto helper = ActionTaskFactory::CreateHelper(runtime);
    if (helper != 0u)
        runtime.AddHelperTask(context, helper, 1u, 0u, 0u);
    runtime.SetHelperOwner(helper, owner);
    TaskPointerCollection::Append(owner + 0x2a0u, helper, runtime);
    runtime.MarkProcessed(context);
    return 1u;
}

std::uint8_t GetItemTask::IsComplete(
    const std::uint32_t object, IGetItemTaskRuntime& runtime) {
    return runtime.ReadByte(object + 0x3cu);
}

void GetItemTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t kind, const std::uint32_t id,
    const std::uint32_t amount, const std::uint32_t context,
    IGetItemTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x4cu, owner);
    runtime.WriteWord(object + 0x40u, kind);
    runtime.WriteWord(object + 0x44u, id);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x48u, amount);
    runtime.WriteWord(object + 0x50u, context);
}

}  // namespace pal4
