#pragma once

#include "pal4/dynamic_strip_object.h"
#include "pal4/strip_visual_block_view.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct DynamicStripVector {
    float x{};
    float y{};
    float z{};
};

class IStripVisualFrameRuntime {
public:
    virtual ~IStripVisualFrameRuntime() = default;
    virtual void TextureSize(std::uint32_t texture,
                             std::uint32_t& width,
                             std::uint32_t& height) = 0;
    virtual void AddTextureReference(std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void SetFrameIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame,
                            DynamicStripVector scale, int combine) = 0;
    virtual void RotateFrame(std::uint32_t frame,
                             DynamicStripVector axis, float angle,
                             int combine) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                DynamicStripVector position,
                                int combine) = 0;
    virtual void UpdateFrameObjects(std::uint32_t frame) = 0;
    virtual void AddChildFrame(std::uint32_t parent,
                               std::uint32_t child) = 0;
};

class IDynamicStripVisualRuntime : public IStripVisualFrameRuntime {
public:
    virtual void CleanupResources(DynamicStripObjectState&) = 0;
    virtual void UpdateFrameProperties(DynamicStripObjectState&) = 0;
};

class IStripVisualBlockRuntime : public IStripVisualFrameRuntime {
public:
    virtual void CleanupResources(StripVisualBlockView) = 0;
    virtual void UpdateFrameProperties(StripVisualBlockView) = 0;
};

class IStripHookLookupRuntime {
public:
    virtual ~IStripHookLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindHook(
        std::uint32_t clump, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t HookFrame(
        std::uint32_t hook) = 0;
    virtual void TextureSize(std::uint32_t texture,
                             std::uint32_t& width,
                             std::uint32_t& height) = 0;
    virtual void AddTextureReference(std::uint32_t texture) = 0;
};

class IDynamicStripHookRuntime : public IStripHookLookupRuntime {
public:
    virtual void CleanupResources(DynamicStripObjectState& state) = 0;
};

class IStripVisualBlockHookRuntime : public IStripHookLookupRuntime {
public:
    virtual void CleanupResources(StripVisualBlockView state) = 0;
};

class IDynamicStripCleanupRuntime {
public:
    virtual ~IDynamicStripCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FirstNode(
        std::uint32_t sentinel) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(
        std::uint32_t node) = 0;
    virtual void UnlinkNode(std::uint32_t node) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyTexture(
        std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyFrame(
        std::uint32_t frame) = 0;
};

class IDynamicStripObjectConfigRuntime {
public:
    virtual ~IDynamicStripObjectConfigRuntime() = default;
    [[nodiscard]] virtual std::string_view ReadTextureName(
        std::uint32_t document, std::string_view key,
        std::uint32_t default_address) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t document, std::string_view key,
        std::int32_t default_value) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t document, std::string_view key,
        float default_value) = 0;
    [[nodiscard]] virtual std::uint32_t FindTexture(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t LoadTexture(
        std::string_view path) = 0;
    virtual void UpdateFrameProperties(DynamicStripObjectState& state) = 0;
    virtual void CleanupCollection(DynamicStripObjectState& state) = 0;
    [[nodiscard]] virtual bool VisualSetup(
        DynamicStripObjectState& state, std::uint32_t texture,
        float width, float life) = 0;
    virtual void ReleaseTexture(std::uint32_t texture) = 0;
    virtual void ReleaseTexturePath(std::string_view path) = 0;
};

class DynamicStripObjectConfigRuntimeStub final
    : public IDynamicStripObjectConfigRuntime {
public:
    [[nodiscard]] std::string_view ReadTextureName(
        std::uint32_t, std::string_view,
        std::uint32_t) override { return {}; }
    [[nodiscard]] std::int32_t ReadInt(
        std::uint32_t, std::string_view,
        std::int32_t value) override { return value; }
    [[nodiscard]] float ReadFloat(
        std::uint32_t, std::string_view,
        float value) override { return value; }
    [[nodiscard]] std::uint32_t FindTexture(
        std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t LoadTexture(
        std::string_view) override { return 0u; }
    void UpdateFrameProperties(DynamicStripObjectState&) override {}
    void CleanupCollection(DynamicStripObjectState&) override {}
    [[nodiscard]] bool VisualSetup(DynamicStripObjectState&,
                                   std::uint32_t, float, float) override {
        return false;
    }
    void ReleaseTexture(std::uint32_t) override {}
    void ReleaseTexturePath(std::string_view) override {}
};

class DynamicStripObjectConfiguration final {
public:
    // IDA 0x64a110. Native AL on gated paths is unspecified.
    static void UpdateFrameProperties(DynamicStripObjectState& state,
                                      IDynamicStripVisualRuntime& runtime);

    static void UpdateFrameProperties(StripVisualBlockView, IStripVisualFrameRuntime&);

    // IDA 0x64a0c0. Native AL after freeing is incidental.
    static void ClearNonHookSegments(DynamicStripObjectState& state,
                                     IDynamicStripCleanupRuntime& runtime);
    static void ClearNonHookSegments(StripVisualBlockView,IDynamicStripCleanupRuntime&);

    // IDA 0x6495f0.
    [[nodiscard]] static std::uint32_t CleanupVisual(
        DynamicStripObjectState& state,
        IDynamicStripCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupVisual(
        StripVisualBlockView state, IDynamicStripCleanupRuntime& runtime);

    // IDA 0x649520.
    [[nodiscard]] static std::uint8_t VisualSetupHookBlade(
        DynamicStripObjectState& state, std::uint32_t texture,
        std::uint32_t clump, float life, bool alternate_hooks,
        IDynamicStripHookRuntime& runtime);
    [[nodiscard]] static std::uint8_t VisualSetupHookBlade(
        StripVisualBlockView state, std::uint32_t texture,
        std::uint32_t clump, float life, bool alternate_hooks,
        IStripVisualBlockHookRuntime& runtime);

    // IDA 0x6492f0.
    [[nodiscard]] static std::uint8_t VisualSetup(
        DynamicStripObjectState& state, std::uint32_t texture,
        float width, float life, IDynamicStripVisualRuntime& runtime);

    [[nodiscard]] static std::uint8_t VisualSetup(
        StripVisualBlockView, std::uint32_t texture, float width, float life,
        IStripVisualBlockRuntime&);

    // IDA 0x60eb20.
    [[nodiscard]] static std::uint8_t Load(
        DynamicStripObjectState& state, std::uint32_t document,
        IDynamicStripObjectConfigRuntime& runtime);
    // IDA 0x60f0a0.
    [[nodiscard]] static std::uint8_t Reset(
        DynamicStripObjectState& state,
        IDynamicStripObjectConfigRuntime& runtime);
};

}  // namespace pal4
