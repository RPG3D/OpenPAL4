#pragma once

#include "pal4/effect_system_object_f_queue.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IEffectHandleRuntime {
public:
    virtual ~IEffectHandleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeSystem() = 0;
    [[nodiscard]] virtual bool Validate(std::uint32_t system,
                                        std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t FindElement(
        std::uint32_t tree, const EffectSystemHandleWords& handle) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class IEffectHandlePropertyRuntime : public IEffectHandleRuntime {
public:
    [[nodiscard]] virtual std::uint32_t RegisterSystem() = 0;
    [[nodiscard]] virtual bool ContainsOwner(std::uint32_t system,
                                             std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::string ReadNameAttribute(
        std::uint32_t attributes) = 0;
};

enum class EffectHandleVectorProperty : std::uint8_t {
    Rotation,
    Scale,
    Position,
};

class IEffectHandleMutationRuntime : public IEffectHandlePropertyRuntime {
public:
    [[nodiscard]] virtual std::int32_t ReadConfigInt(
        std::uint32_t attributes, std::string_view key) = 0;
    virtual void WriteConfigInt(std::uint32_t attributes,
                                std::string_view key,
                                std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ApplyVector(
        EffectHandleVectorProperty property, std::uint32_t dialog,
        const std::array<float, 3u>& value, bool additive) = 0;
    [[nodiscard]] virtual std::uint8_t ApplyParentFrame(
        std::uint32_t dialog, std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint8_t ApplyVisibility(
        std::uint32_t dialog, std::uint32_t visibility) = 0;
    [[nodiscard]] virtual std::uint8_t ApplyFacet(
        std::uint32_t dialog, std::uint32_t first,
        std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint8_t PausePlayback(
        std::uint32_t dialog) = 0;
};

class IEffectHandleReleaseRuntime : public IEffectHandlePropertyRuntime {
public:
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ResetDialogRuntimeState(std::uint32_t dialog) = 0;
    virtual void DispatchOwnerRelease(
        std::uint32_t owner, EffectSystemHandleWords handle,
        std::uint32_t mode) = 0;
    virtual void ReturnEffect(std::uint32_t pool,
                              std::uint32_t dialog,
                              std::uint32_t mode) = 0;
    virtual void EraseEntry(std::uint32_t tree, std::uint32_t node) = 0;
    virtual void ProcessEffectList(std::uint32_t owner) = 0;
};

class IEffectHandleContentRuntime : public IEffectHandleReleaseRuntime {
public:
    // 容器边界：原x86默认实现保留旧存储访问，公共运行时可使用std::map。
    virtual void QueuePendingCompletion(std::uint32_t owner,
        std::uint32_t dialog, float time);
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t PendingNil() = 0;
    [[nodiscard]] virtual std::uint32_t Predecessor(
        std::uint32_t node) = 0;
    virtual void InsertPending(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t dialog, float time) = 0;
    [[nodiscard]] virtual std::array<float, 16u> FrameTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual float StopDialogPlayback(
        std::uint32_t dialog, bool detach_frame) = 0;
    virtual void SetFrameIdentity(std::uint32_t) {}
    virtual void ApplyFrameTransform(
        std::uint32_t, const std::array<float, 16u>&,
        std::uint32_t) {}
    virtual void UpdateFrameObjects(std::uint32_t) {}
};

class IEffectHandleContextRuntime : public IEffectHandlePropertyRuntime {
public:
    virtual void StopContextDialog(std::uint32_t dialog) = 0;
    virtual void DestroyContextDialog(std::uint32_t dialog) = 0;
    [[nodiscard]] virtual float ContextDialogTime(std::uint32_t dialog) = 0;
    virtual void PositionContextDialog(
        std::uint32_t owner, std::uint32_t dialog, float time) = 0;
    virtual void EraseContext(std::uint32_t tree,
                              std::uint32_t node) = 0;
};

class EffectHandle final {
public:
    // IDA 0x639f70: copy the identifier and replace the owner word.
    [[nodiscard]] static EffectSystemHandleWords& CopyData(
        EffectSystemHandleWords& destination,
        const EffectSystemHandleWords& source, std::uint32_t owner);
    // IDA 0x639f90: thunk to the later-addressed reset operation.
    static void ResetThunk(EffectSystemHandleWords& handle);
    // IDA 0x63a6c0: invalidate both handle words.
    static void Reset(EffectSystemHandleWords& handle);
    // IDA 0x639fa0: compare the selected dialog node with the active one.
    [[nodiscard]] static bool DialogNeedsUpdate(
        const EffectSystemHandleWords& handle,
        IEffectHandleRuntime& runtime);
    // IDA 0x63a020/0x63a1d0: read matching dialog attributes.
    [[nodiscard]] static std::string GetName(
        const EffectSystemHandleWords& handle,
        IEffectHandlePropertyRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetFrame(
        const EffectSystemHandleWords& handle,
        IEffectHandlePropertyRuntime& runtime);
    // IDA 0x63a250..0x63a450: transform and visibility forwarding.
    [[nodiscard]] static std::uint8_t SetRotation(
        const EffectSystemHandleWords& handle,
        const std::array<float, 3u>& value, bool additive,
        IEffectHandleMutationRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetScale(
        const EffectSystemHandleWords& handle,
        const std::array<float, 3u>& value, bool additive,
        IEffectHandleMutationRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetPosition(
        const EffectSystemHandleWords& handle,
        const std::array<float, 3u>& value, bool additive,
        IEffectHandleMutationRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetParentFrame(
        const EffectSystemHandleWords& handle, std::uint32_t frame,
        IEffectHandleMutationRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetVisibility(
        const EffectSystemHandleWords& handle, std::uint32_t visibility,
        IEffectHandleMutationRuntime& runtime);
    // IDA 0x63a4d0..0x63a640: integer config and facet operations.
    [[nodiscard]] static std::int32_t GetConfigInt(
        const EffectSystemHandleWords& handle, std::string_view key,
        IEffectHandleMutationRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetConfigInt(
        const EffectSystemHandleWords& handle, std::string_view key,
        std::int32_t value, IEffectHandleMutationRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetFacet(
        const EffectSystemHandleWords& handle,
        std::uint32_t first, std::uint32_t second,
        IEffectHandleMutationRuntime& runtime);
    // IDA 0x63a6d0: pause playback of the matching Dialog.
    [[nodiscard]] static std::uint8_t PausePlayback(
        const EffectSystemHandleWords& handle,
        IEffectHandleMutationRuntime& runtime);
    // IDA 0x63a740: decrement or force-release an owned Dialog effect.
    static void Release(EffectSystemHandleWords& handle, bool force,
                        IEffectHandleReleaseRuntime& runtime);
    // IDA 0x63a820: stop a Dialog and queue its completion record.
    static void UpdateContent(EffectSystemHandleWords& handle,
                              IEffectHandleContentRuntime& runtime);
    // IDA 0x63ada0: test whether the matching +228 context entry exists.
    [[nodiscard]] static bool IsReadyToShow(
        const EffectSystemHandleWords& handle,
        IEffectHandleContextRuntime& runtime);
    // IDA 0x63ae20/0x63aed0: remove or display and retire a context entry.
    static void RemoveContext(EffectSystemHandleWords& handle,
                              IEffectHandleContextRuntime& runtime);
    static void DisplayAndCleanup(EffectSystemHandleWords& handle,
                                  IEffectHandleContextRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t SelectedContextNode(
        const EffectSystemHandleWords& handle,
        IEffectHandleContextRuntime& runtime);
    [[nodiscard]] static std::uint32_t SelectedNode(
        const EffectSystemHandleWords& handle,
        IEffectHandleRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisteredDialog(
        const EffectSystemHandleWords& handle,
        IEffectHandlePropertyRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisteredNode(
        const EffectSystemHandleWords& handle,
        IEffectHandlePropertyRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetVector(
        const EffectSystemHandleWords& handle,
        EffectHandleVectorProperty property,
        const std::array<float, 3u>& value, bool additive,
        IEffectHandleMutationRuntime& runtime);
};

}  // namespace pal4
