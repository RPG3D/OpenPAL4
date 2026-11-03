#pragma once

#include "pal4/effect_handle.h"
#include "pal4/effect_system_group_update.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeDialogStopRuntime;
class NativePlayerTransformRuntime;

class INativeEffectHandleOwnerServices {
public:
    virtual ~INativeEffectHandleOwnerServices() = default;
    virtual std::string ReadNameAttribute(std::uint32_t attributes) = 0;
    virtual void ResetDialogRuntimeState(std::uint32_t dialog) = 0;
    virtual void DispatchOwnerRelease(std::uint32_t owner,
        EffectSystemHandleWords handle, std::uint32_t mode) = 0;
    virtual void ReturnEffect(std::uint32_t pool, std::uint32_t dialog,
        std::uint32_t mode) = 0;
    virtual void ProcessEffectList(std::uint32_t owner) = 0;
};

// Borrows actual initialized singleton/key, active and pending trees. Their
// storage/nils, dialog tasks, frames and owner services must outlive the adapter.
// Implements the existing EffectHandle algorithm's native memory/SDK boundary.
class NativeEffectHandleContentRuntime final : public IEffectHandleContentRuntime,
    public IEffectSystemSecondaryTimerRuntime {
public:
    NativeEffectHandleContentRuntime(NativeBaseObjectMemory& memory,
        std::uint32_t system, std::uint32_t key_nil, std::uint32_t active_nil,
        std::uint32_t pending_nil, NativeDialogStopRuntime& stop,
        NativePlayerTransformRuntime& transforms, INativeEffectHandleOwnerServices& host);
    ~NativeEffectHandleContentRuntime() override;
    std::uint32_t InitializeSystem() override;
    std::uint32_t RegisterSystem() override;
    bool Validate(std::uint32_t system, std::uint32_t owner) override;
    bool ContainsOwner(std::uint32_t system, std::uint32_t owner) override;
    std::uint32_t FindElement(std::uint32_t tree, const EffectSystemHandleWords&) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::string ReadNameAttribute(std::uint32_t attributes) override;
    void ResetDialogRuntimeState(std::uint32_t dialog) override;
    void DispatchOwnerRelease(std::uint32_t owner, EffectSystemHandleWords,
        std::uint32_t mode) override;
    void ReturnEffect(std::uint32_t pool, std::uint32_t dialog, std::uint32_t mode) override;
    void EraseEntry(std::uint32_t tree, std::uint32_t node) override;
    void ProcessEffectList(std::uint32_t owner) override;
    std::uint32_t PendingNil() override;
    std::uint32_t Predecessor(std::uint32_t node) override;
    void InsertPending(std::uint32_t tree, std::uint32_t child, std::uint32_t parent,
        std::uint32_t dialog, float time) override;
    std::array<float,16> FrameTransform(std::uint32_t frame) override;
    float StopDialogPlayback(std::uint32_t dialog, bool detach) override;
    void SetFrameIdentity(std::uint32_t frame) override;
    void ApplyFrameTransform(std::uint32_t frame, const std::array<float,16>& matrix,
        std::uint32_t mode) override;
    void UpdateFrameObjects(std::uint32_t frame) override;
    // IDA631F40: expire the already-stopped Dialogs in this owner's +28 tree.
    void UpdatePending(std::uint32_t owner, double seconds);
    float ReadFloat(std::uint32_t address) override;
    void WriteFloat(std::uint32_t address, float value) override;
    std::uint32_t SharedNil(std::uint32_t tree) override;
    void ResetDialog(std::uint32_t dialog) override;
    void ReturnEffect(std::uint32_t dialog, std::uint32_t mode) override;
    std::uint32_t EraseNode(std::uint32_t tree, std::uint32_t node) override;

private:
    class Memory;
    std::unique_ptr<Memory> memory_;
    std::uint32_t system_, key_nil_, active_nil_, pending_nil_;
    NativeDialogStopRuntime& stop_;
    NativePlayerTransformRuntime& transforms_;
    INativeEffectHandleOwnerServices& host_;
};
}  // namespace pal4
