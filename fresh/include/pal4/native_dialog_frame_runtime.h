#pragma once

#include "pal4/dialog_object.h"
#include "pal4/native_dialog_transform_runtime.h"

namespace pal4 {
class NativeBaseObjectMemory;
class NativeScriptObjectRuntime;
class NativeDialogResetRuntime;
class INativeDialogPauseEffects {
public:
    virtual ~INativeDialogPauseEffects() = default;
    // Effect virtual +56, distinct from SetEffectMode (virtual +48).
    virtual void PauseEffect(std::uint32_t effect) = 0;
};

// Borrows the task owner, reset runtime and effect dispatcher. Tracks an SDK
// RwFrame directly; callers must retain the tracked frame until tracking stops.
class NativeDialogFrameRuntime final : public IDialogObjectPauseRuntime,
    public IDialogObjectUpdateRuntime {
public:
    NativeDialogFrameRuntime(NativeBaseObjectMemory&, NativeScriptObjectRuntime&,
        NativeDialogResetRuntime&, INativeDialogPauseEffects&);
    std::uint32_t ReadEffectHandle(std::uint32_t record) override;
    void PauseEffect(std::uint32_t effect) override;
    void RefreshFrame(DialogObjectState&) override;
    void BeginWaiting(std::uint32_t container) override;
    int UpdateTaskContainer(std::uint32_t container, float elapsed) override;
    bool TaskContainerReady(std::uint32_t container) override;
    int ResetRuntimeState(DialogObjectState&) override;
    std::array<float,3> ObjectPosition(std::uint32_t frame) override;
    int SetAbsolutePosition(DialogObjectState&,
        const std::array<float,3>& position) override;
private:
    NativeBaseObjectMemory& memory_;
    NativeScriptObjectRuntime& tasks_;
    NativeDialogResetRuntime& reset_;
    INativeDialogPauseEffects& effects_;
    NativeDialogTransformRuntime transforms_;
};
}  // namespace pal4
