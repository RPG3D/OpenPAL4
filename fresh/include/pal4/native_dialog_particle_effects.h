#pragma once
#include "pal4/dialog_object.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEffectTextManager;
class NativeParticleEffectFactory;

class INativeDialogParticleQueues {
public:
    virtual ~INativeDialogParticleQueues()=default;
    virtual void QueueEffect(std::uint32_t parent,std::uint32_t effect)=0;
    virtual void RemoveEffect(std::uint32_t parent,std::uint32_t effect)=0;
};

// Binds one Dialog's templates to the concrete particle factory and SDK frames.
// All services are borrowed. Stop referencing tasks before Stop/destruction.
class NativeDialogParticleEffects final {
public:
    NativeDialogParticleEffects(NativeBaseObjectMemory&,NativeEffectTextManager&,
        NativeParticleEffectFactory&,INativeDialogParticleQueues&);
    ~NativeDialogParticleEffects();
    NativeDialogParticleEffects(const NativeDialogParticleEffects&)=delete;
    NativeDialogParticleEffects& operator=(const NativeDialogParticleEffects&)=delete;
    // Requires the caller's Dialog frame. Rolls back partial activation on
    // failure/exception. Already-bound records must be stopped before retry.
    [[nodiscard]] bool Activate(DialogObjectState&,std::uint32_t object);
    void Stop(DialogObjectState&);
    // Separate endpoints preserve Queue on activation and Remove on cleanup,
    // despite the shared interfaces' common ProcessEffectEvent spelling.
    [[nodiscard]] IDialogObjectActivationRuntime& ActivationRuntime();
    [[nodiscard]] IDialogObjectEffectCleanupRuntime& CleanupRuntime();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
