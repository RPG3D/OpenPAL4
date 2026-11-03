#pragma once
#include "pal4/native_dialog_pre_delete_runtime.h"
#include "pal4/native_dialog_playback_runtime.h"
#include "pal4/native_dialog_frame_runtime.h"
#include "pal4/native_dialog_stop_runtime.h"

namespace pal4 {
class NativeBaseObjectMemory;
class NativeParticleEffectFactory;
class INativeDialogParticleQueues;

// Concrete particle endpoints for shared Dialog reset, pre-render and resume.
// Borrows all services; only particle payloads belong to this dispatcher.
class NativeDialogParticlePlayback final : public INativeDialogDeleteEffects,
    public INativeDialogRestartEffects, public INativeDialogPauseEffects,
    public INativeDialogStopEffects {
public:
    NativeDialogParticlePlayback(NativeBaseObjectMemory&,NativeParticleEffectFactory&,
                                INativeDialogParticleQueues&);
    bool EffectsEnabled() override;
    bool EffectIsActive(std::uint32_t) override;
    void SetEffectMode(std::uint32_t,std::uint32_t) override;
    void ReleaseEffectReferences(std::uint32_t) override;
    void ProcessEffectEvent(std::uint32_t,std::uint32_t) override;
    bool RestartEffect(std::uint32_t) override;
    void PauseEffect(std::uint32_t) override;
    std::uint32_t EffectFrame(std::uint32_t) override;
    void DestroyEffect(std::uint32_t) override;
    void InitializeEffectSystem() override;
    double EffectValue(std::uint32_t) override;
private:
    NativeBaseObjectMemory& memory_;
    NativeParticleEffectFactory& objects_;
    INativeDialogParticleQueues& queues_;
};
}
