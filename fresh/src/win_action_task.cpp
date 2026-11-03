#include "pal4/win_action_task.h"

#include "pal4/check_protect_task.h"
#include "pal4/script_task_base.h"

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 3> kTiming{1u, 0u, 0u};
constexpr std::uint32_t kEmptyText = 0x841964u;

std::uint32_t GlobalText(IWinActionRuntime& runtime,
                         const std::uint32_t address) {
    const auto text = runtime.ReadWord(address);
    return text == 0u ? kEmptyText : text;
}

std::uint8_t AssignLegacyVictoryText(const std::uint32_t child,
                               const std::uint32_t header,
                               IWinActionRuntime& runtime) {
    const auto destination = child + 64u;
    const auto length = runtime.ReadWord(header + 8u);
    if (destination == header) {
        (void)runtime.EraseText(destination, length, 0xffffffffu);
        return runtime.EraseText(destination, 0u, 0u);
    }
    if (length != 0u) {
        const auto data = GlobalText(runtime, header + 4u);
        const auto references = runtime.ReadByte(data - 1u);
        if (references < 0xfeu) {
            runtime.SetTextErrorFlag(destination, 1u);
            const auto shared_data = GlobalText(runtime, header + 4u);
            runtime.WriteWord(destination + 4u, shared_data);
            runtime.WriteWord(destination + 8u,
                              runtime.ReadWord(header + 8u));
            runtime.WriteWord(destination + 12u,
                              runtime.ReadWord(header + 12u));
            runtime.WriteByte(shared_data - 1u,
                              static_cast<std::uint8_t>(references + 1u));
            return static_cast<std::uint8_t>(shared_data);
        }
    }
    if (!runtime.AllocateText(destination, length, 1u)) return 0u;
    runtime.CopyText(destination, GlobalText(runtime, header + 4u), length);
    runtime.WriteWord(destination + 8u, length);
    runtime.WriteByte(runtime.ReadWord(destination + 4u) + length, 0u);
    return static_cast<std::uint8_t>(length);
}

}  // namespace

std::uint8_t IWinActionRuntime::AssignVictoryText(std::uint32_t child,std::uint32_t header){
    return AssignLegacyVictoryText(child,header,*this);
}

WinActionChildFactorySlot* WinActionSecondChildFactory::Initialize(
    WinActionChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

WinActionChildFactorySlot* WinActionThirdChildFactory::Initialize(
    WinActionChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

std::uint8_t WinActionTask::Start(const std::uint32_t task,
                                  IWinActionRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 44u);
    runtime.RestoreRequestedModel(actor);

    const auto position_text = GlobalText(runtime, 0x8f9798u);
    const auto position_child = runtime.CreateChildTask(
        CheckProtectSubtaskFactory::kPositionVtable);
    if (position_child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, position_child,
                                  position_text, kTiming);
    const auto position = runtime.ActorPosition(runtime.ReadWord(task + 44u));
    runtime.WriteWord(position_child + 72u, runtime.ReadWord(task + 44u));
    runtime.WriteWord(position_child + 60u, position[0]);
    runtime.WriteWord(position_child + 64u, position[1]);
    runtime.WriteWord(position_child + 68u, position[2]);

    const auto second_text = GlobalText(runtime, 0x8f9770u);
    WinActionChildFactorySlot second_factory;
    (void)WinActionSecondChildFactory::Initialize(&second_factory);
    const auto second_child = runtime.CreateChildTask(second_factory.vtable);
    if (second_child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, second_child,
                                  second_text, kTiming);
    runtime.WriteWord(second_child + 60u, runtime.ReadWord(task + 44u));

    const auto third_text = GlobalText(runtime, 0x8f95d0u);
    WinActionChildFactorySlot third_factory;
    (void)WinActionThirdChildFactory::Initialize(&third_factory);
    const auto third_child = runtime.CreateChildTask(third_factory.vtable);
    if (third_child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, third_child,
                                  third_text, kTiming);
    const auto sample = runtime.RandomNumber() % 1000u;
    runtime.WriteWord(third_child + 60u, runtime.ReadWord(task + 44u));
    const auto selected_header = sample < 500u ? 0x8fa2d8u : 0x8fa2c8u;
    return runtime.AssignVictoryText(third_child, selected_header);
}

std::uint32_t WinActionTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t WinActionTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

}  // namespace pal4
