#pragma once

#include "pal4/dynamic_strip_object.h"
#include "pal4/strip_visual_block_view.h"

#include <cstdint>

namespace pal4 {

class IStripResourceStorageRuntime {
public:
    virtual ~IStripResourceStorageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IDynamicStripResourceBlockRuntime : public IStripResourceStorageRuntime {
public:
    virtual void CleanupVisual(DynamicStripObjectState& state) = 0;
};

class DynamicStripResourceBlockRuntimeStub final
    : public IDynamicStripResourceBlockRuntime {
public:
    void CleanupVisual(DynamicStripObjectState&) override {}
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void Free(std::uint32_t) override {}
};

class DynamicStripResourceBlock final {
public:
    // IDA 0x60fa00; resource block is embedded at object +52.
    static void Cleanup(DynamicStripObjectState& state,
                        IDynamicStripResourceBlockRuntime& runtime);
    // Storage-only tail of 60FA00, after the caller's visual cleanup. Handles
    // retained nodes even when the visual was inactive; sentinel zero is safe.
    static void ReleaseStorage(StripVisualBlockView block,
                               IStripResourceStorageRuntime& runtime);
};

}  // namespace pal4
