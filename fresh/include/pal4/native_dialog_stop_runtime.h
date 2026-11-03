#pragma once

#include "pal4/dialog_object.h"

namespace pal4 {
class NativeBaseObjectMemory;
class NativeScriptObjectRuntime;

class INativeDialogStopEffects {
public:
    virtual ~INativeDialogStopEffects() = default;
    virtual void InitializeEffectSystem() = 0;
    virtual void SetEffectMode(std::uint32_t effect, std::uint32_t mode) = 0;
    virtual double EffectValue(std::uint32_t effect) = 0;
};

// Borrows the task runtime which registered this dialog's tasks, their native
// allocation owner, and the effect dispatcher. No original EXE vtables are called.
class NativeDialogStopRuntime final : public IDialogObjectStopRuntime {
public:
    NativeDialogStopRuntime(NativeBaseObjectMemory& memory,
        NativeScriptObjectRuntime& tasks, INativeDialogStopEffects& effects)
        : memory_(memory), tasks_(tasks), effects_(effects) {}

    void CleanupObjectCollection(std::uint32_t container) override;
    void DestroyTaskContainer(std::uint32_t container) override;
    void Free(std::uint32_t address) override;
    std::uint32_t FrameParent(std::uint32_t frame) override;
    void RemoveFrameChild(std::uint32_t frame) override;
    std::uint32_t ReadWord(std::uint32_t address, std::uint32_t offset) override;
    void WriteWord(std::uint32_t address, std::uint32_t offset,
        std::uint32_t value) override;
    void InitializeEffectSystem() override;
    void SetEffectMode(std::uint32_t effect, std::uint32_t mode) override;
    double EffectValue(std::uint32_t effect) override;

private:
    NativeBaseObjectMemory& memory_;
    NativeScriptObjectRuntime& tasks_;
    INativeDialogStopEffects& effects_;
};
}  // namespace pal4
