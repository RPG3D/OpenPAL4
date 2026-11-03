#pragma once

#include "pal4/dialog_object.h"

namespace pal4 {
class NativeBaseObjectMemory;
class NativeScriptObjectRuntime;

class INativeDialogResetEffects {
public:
    virtual ~INativeDialogResetEffects() = default;
    virtual bool EffectsEnabled() = 0;
    virtual bool EffectIsActive(std::uint32_t effect) = 0;
    virtual void SetEffectMode(std::uint32_t effect, std::uint32_t mode) = 0;
    virtual void ReleaseEffectReferences(std::uint32_t effect) = 0;
    virtual void ProcessEffectEvent(std::uint32_t owner, std::uint32_t effect) = 0;
};

// Borrows the real task/allocation owners and an explicit effect dispatcher.
// Use with DialogObject::ResetRuntimeState, including pending-pool retirement.
class NativeDialogResetRuntime final : public IDialogObjectResetRuntime {
public:
    NativeDialogResetRuntime(NativeBaseObjectMemory& memory,
        NativeScriptObjectRuntime& tasks, INativeDialogResetEffects& effects)
        : memory_(memory), tasks_(tasks), effects_(effects) {}
    void CleanupObjectCollection(std::uint32_t container) override;
    void DestroyTaskContainer(std::uint32_t container) override;
    void Free(std::uint32_t address) override;
    std::uint32_t ReadWord(std::uint32_t address, std::uint32_t offset) override;
    void WriteWord(std::uint32_t address, std::uint32_t offset,
        std::uint32_t value) override;
    bool EffectsEnabled() override;
    bool EffectIsActive(std::uint32_t effect) override;
    void SetEffectMode(std::uint32_t effect, std::uint32_t mode) override;
    void ReleaseEffectReferences(std::uint32_t effect) override;
    void ProcessEffectEvent(std::uint32_t owner, std::uint32_t effect) override;
private:
    NativeBaseObjectMemory& memory_;
    NativeScriptObjectRuntime& tasks_;
    INativeDialogResetEffects& effects_;
};
}  // namespace pal4
