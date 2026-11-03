#include "pal4/gob_machine.h"
#include "pal4/gob_machine_control.h"

namespace pal4 {

std::uint32_t GobMachine::StartAndSaveTransform(
    const std::uint32_t object, IGobMachineControlRuntime& runtime) {
    runtime.StartTaskRun(object);
    runtime.MarkEventProcessed(object + kPrimaryTasksOffset);
    runtime.WriteWord(object + 0x278u, 0u);
    runtime.WriteWord(object + 0x27cu, 1u);
    for (std::uint32_t index = 0; index < 3u; ++index)
        runtime.WriteWord(object + 0x288u + index * 4u,
                          runtime.ReadWord(object + 0xacu + index * 4u));
    const auto first_rotation = runtime.ReadWord(object + 0xa0u);
    runtime.WriteWord(object + 0x294u, first_rotation);
    for (std::uint32_t index = 1; index < 3u; ++index)
        runtime.WriteWord(object + 0x294u + index * 4u,
                          runtime.ReadWord(object + 0xa0u + index * 4u));
    return first_rotation;
}

std::uint32_t GobMachine::ResetTriggeredRuntime(
    const std::uint32_t object, IGobMachineControlRuntime& runtime) {
    if (runtime.ReadWord(object + 0x27cu) != 0u) {
        runtime.SetPosition(object, object + 0x288u, 0u);
        runtime.SetRotation(object, object + 0x294u, 0u);
    }
    (void)runtime.ClearTaskCollection(object + kPrimaryTasksOffset);
    const auto result = runtime.ClearTaskCollection(
        object + kContextTasksOffset);
    runtime.WriteWord(object + 0x278u, 0u);
    runtime.WriteWord(object + 0x27cu, 0u);
    return result;
}

bool GobMachine::AreTaskContainersReady(
    const std::uint32_t object, IGobMachineControlRuntime& runtime) {
    return runtime.ReadByte(object + 0x264u) != 0u &&
           runtime.ReadByte(object + 0x274u) != 0u;
}

namespace {

std::uint8_t InvokeTaskRange(const std::uint32_t object,
                             const std::uint32_t slot,
                             IGobMachineControlRuntime& runtime) {
    const auto ready = runtime.ReadByte(object + 0x264u);
    if (ready != 0u) return ready;
    auto cursor = runtime.ReadWord(object + 0x2a4u);
    auto end = runtime.ReadWord(object + 0x2a8u);
    while (cursor != end) {
        runtime.InvokeTaskSlot(runtime.ReadWord(cursor), slot);
        end = runtime.ReadWord(object + 0x2a8u);
        cursor += 4u;
    }
    return static_cast<std::uint8_t>(end);
}

}  // namespace

std::uint8_t GobMachine::InvokeTaskSlot28Range(
    const std::uint32_t object, IGobMachineControlRuntime& runtime) {
    return InvokeTaskRange(object, 0x1cu, runtime);
}

std::uint8_t GobMachine::InvokeTaskSlot32Range(
    const std::uint32_t object, IGobMachineControlRuntime& runtime) {
    return InvokeTaskRange(object, 0x20u, runtime);
}

std::uint32_t GobMachine::GetMachineConfigNode(
    const std::uint32_t object, IGobMachineConfigRuntime& runtime) {
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) return 0u;
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters == 0u) return 0u;
    return runtime.FindXmlChild(parameters, runtime.MachineConfigSection());
}

std::uint32_t GobMachine::LoadMainConfigAndTaskFlags(
    const std::uint32_t object, IGobMachineConfigRuntime& runtime) {
    runtime.ApplyBaseParameters(object);
    const auto machine = GetMachineConfigNode(object, runtime);
    if (machine == 0u) return 0u;

    runtime.WriteWord(object + 0x280u, static_cast<std::uint32_t>(
        runtime.ReadXmlInt(machine, "PAL4_GameObject-machine-repeat", 0)));
    runtime.WriteWord(object + 0x284u, static_cast<std::uint32_t>(
        runtime.ReadXmlInt(machine, "PAL4_GameObject-machine-auto-touch", 0)));
    runtime.WriteWord(object + 0x2b0u, 0u);
    const auto tasks = runtime.FindXmlChild(
        machine, "PAL4_GameObject-machine-tasks");
    if (tasks == 0u) return 0u;
    const auto count = runtime.XmlChildCount(tasks);
    if (count <= 0) return tasks;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto task = runtime.XmlChildAt(tasks, index);
        if (task == 0u) return 0u;
        const auto type = runtime.ReadXmlInt(task, "PAL4_GOMTask_type", -1);
        if (type == 3) {
            runtime.WriteWord(object + 0x2b8u, 1u);
            runtime.AssignActionName(object + 0x2bcu,
                runtime.ReadXmlString(task, "PAL4_GOMTask_action_name", ""));
        } else if (type == 6) {
            runtime.WriteWord(object + 0x2b0u, 1u);
        }
    }
    return static_cast<std::uint32_t>(count);
}

std::uint32_t GobMachine::ApplyVisibleFlagAndRepeatTrigger(
    const std::uint32_t object, const std::uint32_t visible,
    IGobMachineVisibilityRuntime& runtime) {
    runtime.SetVisible(object, visible);
    (void)runtime.ClearTaskCollection(object + kPrimaryTasksOffset);
    (void)runtime.ClearTaskCollection(object + kContextTasksOffset);
    const auto repeat = runtime.ReadWord(object + 0x284u);
    runtime.WriteWord(object + 0x278u, 0u);
    runtime.WriteWord(object + 0x27cu, 0u);
    if (repeat != 0u && visible != 0u)
        return runtime.TriggerTaskRun(object, {0.0f, 0.0f, 0.0f});
    return 0u;
}

}  // namespace pal4
