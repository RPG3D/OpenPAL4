#include "pal4/dialog_object.h"

#include <string>

namespace pal4 {
namespace {
std::string ErrorMessage(DialogTaskInitError code, std::string_view name) {
    // Native exception text is GBK in launch.exe.
    switch (code) {
    case DialogTaskInitError::MissingManager:
        return "vtmMgr\xce\xaa\xbf\xd5\xa3\xac\xc8\xce\xce\xf1"
               "\xd7\xbc\xb1\xb8\xca\xa7\xb0\xdc\xa3\xa1";
    case DialogTaskInitError::EmptyClassName:
        return "\xc8\xce\xce\xf1\xc3\xfb\xce\xaa\xbf\xd5\xa3\xac"
               "\xb4\xb4\xbd\xa8\xc8\xce\xce\xf1\xca\xa7\xb0\xdc"
               "\xa3\xa1";
    case DialogTaskInitError::CreateTaskFailed:
        return "\xb4\xb4\xbd\xa8\xcc\xd8\xd0\xa7\xc8\xce\xce\xf1"
               "\xca\xa7\xb0\xdc\xa3\xa1\xc8\xce\xce\xf1\xc3\xfb"
               "\xa3\xba" + std::string(name);
    case DialogTaskInitError::EffectPlaybackFailed:
        return "\xb4\xb4\xbd\xa8\xcc\xd8\xd0\xa7\xc8\xce\xce\xf1"
               "\xca\xa7\xb0\xdc\xa3\xac\xcc\xd8\xd0\xa7\xb2\xa5"
               "\xb7\xc5\xca\xa7\xb0\xdc\xa3\xa1";
    }
    return "Unknown dialog task error";
}
}  // namespace

DialogTaskInitException::DialogTaskInitException(
    DialogTaskInitError code, std::string_view name)
    : std::runtime_error(ErrorMessage(code, name)), code_(code) {}

bool DialogObject::InitializeTasks(
    DialogObjectState& state, std::uint32_t object,
    IDialogObjectTaskRuntime& runtime) {
    const auto manager = state.words[2u];
    if (manager == 0u)
        throw DialogTaskInitException(DialogTaskInitError::MissingManager);

    auto xml_slot = state.words[32u];
    for (auto record = state.words[24u]; record != state.words[25u];
         record += 8u, xml_slot += 4u) {
        const auto xml = runtime.ReadWord(xml_slot, 0u);
        const auto enabled = runtime.EffectsEnabled();
        std::uint32_t effect_address = 0u;
        if (!enabled) {
            const auto index = runtime.ReadWord(record, 0u);
            effect_address = state.words[28u] + 20u * index;
            if (runtime.EffectIsActive(
                    runtime.ReadWord(effect_address, 16u))) {
                runtime.WriteWord(record, 4u, 0u);
                continue;
            }
        }

        const DialogTaskTiming timing{
            runtime.IntAttribute(xml, "Attr_Task_Wait_Mode", !enabled)
                .value_or(0),
            runtime.FloatAttribute(xml, "Attr_Task_Wait_Time")
                .value_or(0.0f),
            runtime.FloatAttribute(xml, "Attr_Task_Expect_Time")
                .value_or(0.0f)};
        const auto name = runtime.StringAttribute(xml, "Attr_Class_Name")
                              .value_or("");
        if (!enabled && name.empty())
            throw DialogTaskInitException(DialogTaskInitError::EmptyClassName);

        const auto task = runtime.CreateTask(xml);
        if (task != 0u)
            runtime.AddTaskAndInitialize(manager, task, name, timing);
        runtime.WriteWord(record, 4u, task);
        if (task == 0u) {
            throw DialogTaskInitException(enabled
                ? DialogTaskInitError::CreateTaskFailed
                : DialogTaskInitError::EffectPlaybackFailed, name);
        }
        if (enabled) {
            const auto index = runtime.ReadWord(record, 0u);
            effect_address = state.words[28u] + 20u * index;
        }
        runtime.WriteWord(task, 64u,
                          runtime.ReadWord(effect_address, 16u));
        runtime.BindDialogObject(task, object);
        runtime.PrepareTask(task);
        runtime.StartTask(task);
    }
    return true;
}

std::uint32_t DialogObject::ResetTaskSlots(
    DialogObjectState& state, IDialogObjectTaskRuntime& runtime) {
    for (auto record = state.words[24u]; record != state.words[25u];
         record += 8u)
        runtime.WriteWord(record, 4u, 0u);
    return state.words[25u];
}

}  // namespace pal4
