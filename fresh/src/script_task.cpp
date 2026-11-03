#include "pal4/script_task.h"

#include "pal4/action_task_object.h"
#include "pal4/task_pointer_collection.h"

namespace pal4 {

std::uint32_t ScriptTask::Construct(
    const std::uint32_t object, IScriptTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteByte(object + 0x40u, runtime.TransientInitialByte());
    runtime.WriteWord(object + 0x44u, 0u);
    runtime.WriteWord(object + 0x48u, 0u);
    runtime.WriteWord(object + 0x4cu, 0u);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x54u, 0u);
    runtime.WriteWord(object + 0x58u, 0u);
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t ScriptTask::Destruct(
    const std::uint32_t object, IScriptTaskRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x54u, 0u);
    runtime.WriteWord(object + 0x58u, 0u);
    const auto data = runtime.ReadWord(object + 0x44u);
    if (data != 0u) {
        const auto count = runtime.ReadByte(data - 1u);
        if (count == 0u || count == 0xffu)
            runtime.FreeMemory(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    for (const auto offset : {0x44u, 0x48u, 0x4cu})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteWord(object, 0x8486fcu);
    return runtime.BaseDestruct(object);
}

std::uint8_t ScriptTask::Enter(
    const std::uint32_t object, IScriptTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x54u) == 0u) return 0u;
    if (runtime.ReadWord(object + 0x48u) == 0u) {
        runtime.WriteByte(object + 0x3cu, 1u);
        return 0u;
    }
    if (!runtime.ActorControlActive() && runtime.ScriptSystemBusy()) {
        runtime.WriteByte(object + 0x3cu, 1u);
        return 0u;
    }

    runtime.InitializeScript();
    const auto function_name = runtime.ReadCString(
        runtime.ReadWord(object + 0x44u));
    const std::string full_name =
        std::string(kScriptPath) + "[" + function_name + "]";
    if (!runtime.ScriptCacheContains(full_name)) {
        const auto arena_context = runtime.ArenaContext();
        const auto interpreter = runtime.CreateInterpreter(
            runtime.IsCsbMode(), kScriptPath, full_name,
            function_name, arena_context);
        if (interpreter != 0u &&
            runtime.AddScriptModule(interpreter)) {
            runtime.PushStackModule(interpreter);
            runtime.ReleaseInterpreter(interpreter);
        } else {
            runtime.ShowMissingScriptError();
        }
    }
    runtime.CleanupScriptFiles();
    if (runtime.ReadWord(object + 0x50u) == 0u)
        runtime.WriteByte(object + 0x3cu, 1u);
    return 1u;
}

std::uint8_t ScriptTask::Update(
    const std::uint32_t object, IScriptTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x54u) == 0u) {
        runtime.WriteByte(object + 0x3cu, 0u);
        return 0u;
    }
    if (runtime.ReadByte(object + 0x3cu) != 0u) return 0u;
    const auto cached = runtime.FindCachedScript(kScriptPath);
    if (cached == 0u || cached == 0xffffffffu)
        runtime.WriteByte(object + 0x3cu, 1u);
    return 1u;
}

std::uint8_t ScriptTask::SpawnFollowup(
    const std::uint32_t object, IScriptTaskSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x58u);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x54u);
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

std::uint32_t ScriptTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t function_name, const std::uint32_t wait,
    const std::uint32_t context, IScriptTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x54u, owner);
    runtime.WriteByte(object + 0x3cu, 0u);
    const auto length = runtime.CStringLength(function_name);
    if (length > 0xfffffffdu) runtime.ThrowLength();
    const auto record = object + 0x40u;
    const auto data = runtime.ReadWord(record + 4u);
    bool shared = false;
    if (data != 0u) {
        const auto count = runtime.ReadByte(data - 1u);
        shared = count != 0u && count != 0xffu;
        if (shared && length == 0u) {
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(count - 1u));
            for (const auto offset : {4u, 8u, 12u})
                runtime.WriteWord(record + offset, 0u);
            runtime.WriteWord(object + 0x50u, wait);
            runtime.WriteWord(object + 0x58u, context);
            return data;
        }
    }
    if (length == 0u) {
        (void)runtime.SetErrorFlag(record, 1u);
        runtime.WriteWord(object + 0x50u, wait);
        runtime.WriteWord(object + 0x58u, context);
        return wait;
    }
    if (shared) {
        runtime.ReallocateText(record, length);
    } else {
        const auto capacity = runtime.ReadWord(record + 12u);
        if (capacity > 0x1fu || capacity < length) {
            (void)runtime.SetErrorFlag(record, 1u);
            runtime.ReallocateText(record, length);
        }
    }
    const auto destination = runtime.ReadWord(record + 4u);
    runtime.CopyBytes(destination, function_name, length);
    runtime.WriteWord(record + 8u, length);
    runtime.WriteByte(destination + length, 0u);
    runtime.WriteWord(object + 0x50u, wait);
    runtime.WriteWord(object + 0x58u, context);
    return context;
}

}  // namespace pal4
