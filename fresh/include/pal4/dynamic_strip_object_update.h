#pragma once

#include "pal4/dynamic_strip_object.h"
#include "pal4/strip_visual_block_view.h"

#include <cstdint>
#include <array>

namespace pal4 {

class IDynamicStripObjectUpdateRuntime {
public:
    virtual ~IDynamicStripObjectUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint8_t UpdateVisual(
        DynamicStripObjectState& state, double frame_time,
        std::uint8_t mode) = 0;
};

struct DynamicStripSegment {
    std::array<float, 3u> first{};
    std::array<float, 3u> second{};
    float remaining{};
};
static_assert(sizeof(DynamicStripSegment) == 28u);

class IDynamicStripSegmentListRuntime {
public:
    virtual ~IDynamicStripSegmentListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void CopySegment(std::uint32_t address,
                             const DynamicStripSegment& segment) = 0;
};

class DynamicStripSegmentList final {
public:
    // IDA 0x64a1e0. Appends a 36-byte linked node to the given sentinel.
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t sentinel, std::uint32_t& count,
        const DynamicStripSegment& segment,
        IDynamicStripSegmentListRuntime& runtime);
};

class IDynamicStripVisualUpdateRuntime {
public:
    virtual ~IDynamicStripVisualUpdateRuntime() = default;
    [[nodiscard]] virtual std::array<float, 3u> FramePosition(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t FirstNode(
        std::uint32_t sentinel) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual float RemainingLife(
        std::uint32_t node) = 0;
    virtual void SetRemainingLife(std::uint32_t node, float value) = 0;
    virtual void RemoveNode(std::uint32_t node) = 0;
    virtual void InsertFront(std::uint32_t sentinel,
                             const DynamicStripSegment& segment) = 0;
};

class DynamicStripObjectUpdateRuntimeStub final
    : public IDynamicStripObjectUpdateRuntime {
public:
    [[nodiscard]] std::uint8_t UpdateVisual(
        DynamicStripObjectState&, double,
        std::uint8_t) override { return 0u; }
};

class DynamicStripObjectUpdater final {
public:
    // IDA 0x6496a0. Return is a semantic insertion flag; native callers
    // do not use the incidental full EAX residue.
    [[nodiscard]] static std::uint8_t UpdateVisual(
        DynamicStripObjectState& state, double frame_time,
        std::uint8_t mode, IDynamicStripVisualUpdateRuntime& runtime);
    [[nodiscard]] static std::uint8_t UpdateVisual(
        StripVisualBlockView state, double frame_time,
        std::uint8_t mode, IDynamicStripVisualUpdateRuntime& runtime);

    // IDA 0x60f170.
    [[nodiscard]] static std::uint8_t Update(
        DynamicStripObjectState& state, double frame_time,
        IDynamicStripObjectUpdateRuntime& runtime);
};

}  // namespace pal4
