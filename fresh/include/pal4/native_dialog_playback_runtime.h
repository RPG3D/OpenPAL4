#pragma once
#include "pal4/dialog_object.h"
#include "pal4/native_dialog_transform_runtime.h"

namespace pal4 {
class NativeBaseObjectMemory;
class NativeScriptObjectRuntime;
class NativeDialogResetRuntime;
class INativeDialogRestartEffects {
public:
    virtual ~INativeDialogRestartEffects() = default;
    virtual void SetEffectMode(std::uint32_t effect, std::uint32_t mode) = 0;
    virtual bool RestartEffect(std::uint32_t effect) = 0;
};

// Owns no Dialog/resources. Borrows initialized factory/dispatch services and
// the same task owner used by frame/reset. Shared algorithms own sequencing.
class NativeDialogPlaybackRuntime final : public IDialogObjectResumeRuntime,
    public IDialogObjectStartRuntime {
public:
    NativeDialogPlaybackRuntime(NativeBaseObjectMemory&, NativeScriptObjectRuntime&,
        NativeDialogResetRuntime&, IDialogObjectActivationRuntime&,
        IDialogObjectTaskRuntime&, IDialogObjectEffectCleanupRuntime&,
        INativeDialogRestartEffects&);
    void ResetBeforePlayback(DialogObjectState&) override;
    void DestroyFrame(std::uint32_t) override;
    std::uint32_t CreateFrame() override;
    void RefreshFrame(DialogObjectState&) override;
    void DestroyTaskContainer(std::uint32_t) override;
    void Free(std::uint32_t) override;
    std::uint32_t Allocate(std::uint32_t) override;
    std::uint32_t InitializeTaskContainer(std::uint32_t) override;
    bool ActivateEffects(DialogObjectState&) override;
    void StopEffects(DialogObjectState&) override;
    bool InitializeTasks(DialogObjectState&) override;
    void ResetTaskSlots(DialogObjectState&) override;
    std::uint32_t ReadEffectHandle(std::uint32_t) override;
    void SetEffectMode(std::uint32_t,std::uint32_t) override;
    bool EffectsEnabled() override;
    bool EffectIsActive(std::uint32_t) override;
    void WriteEffectWord(std::uint32_t,std::uint32_t,std::uint32_t) override;
    bool RestartEffect(std::uint32_t) override;
    // Resume requeues through activation; StopEffects uses cleanup separately.
    void ProcessEffectEvent(std::uint32_t,std::uint32_t) override;
    std::uint32_t EffectFrame(std::uint32_t) override;
    void AddFrameChild(std::uint32_t,std::uint32_t) override;
    int Random15() override;
    bool ResumePlayback(DialogObjectState&) override;
    bool PreparePlayback(DialogObjectState&) override;
private:
    NativeBaseObjectMemory& memory_;
    NativeScriptObjectRuntime& tasks_;
    NativeDialogResetRuntime& reset_;
    IDialogObjectActivationRuntime& activation_;
    IDialogObjectTaskRuntime& factory_;
    IDialogObjectEffectCleanupRuntime& cleanup_;
    INativeDialogRestartEffects& restart_;
    NativeDialogTransformRuntime transforms_;
};
}  // namespace pal4
