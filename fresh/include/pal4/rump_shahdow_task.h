#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct RumpShahdowTaskState final {
    std::array<std::uint32_t, 46u> words{};
};
static_assert(sizeof(RumpShahdowTaskState) == 0xb8u);

struct RumpShahdowRecord final {
    float age = 0.0f;
    std::uint8_t original_alpha = 0u;
    std::array<std::uint8_t, 3u> padding{};
    std::uint32_t child = 0u;
};
static_assert(sizeof(RumpShahdowRecord) == 0xcu);

using RumpShahdowColor = std::array<std::uint8_t, 4u>;
using RumpShahdowTransform = std::array<float, 16u>;

class IRumpShahdowTaskRuntime {
public:
    virtual ~IRumpShahdowTaskRuntime() = default;
    virtual void ConstructBase(RumpShahdowTaskState& state) = 0;
    virtual void DestroyBase(RumpShahdowTaskState& state) = 0;
    virtual void FreeCollectionStorage(RumpShahdowTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveFacet(
        std::uint32_t object, std::string_view class_name) = 0;
    virtual void PrepareOwner(std::uint32_t owner) = 0;
    [[nodiscard]] virtual RumpShahdowColor ReadColor(std::uint32_t facet) = 0;
    virtual void WriteColor(std::uint32_t facet,
                            const RumpShahdowColor& color) = 0;
    [[nodiscard]] virtual bool ValidateCount(std::uint32_t facet,
                                              std::int32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChild(std::uint32_t facet,
                                                     std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateRecord(std::uint32_t bytes) = 0;
    virtual void FreeRecord(std::uint32_t record) = 0;
    [[nodiscard]] virtual RumpShahdowRecord ReadRecord(
        std::uint32_t record) = 0;
    virtual void WriteRecord(std::uint32_t record,
                             const RumpShahdowRecord& value) = 0;
    virtual void AppendRecord(RumpShahdowTaskState& state,
                              std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t RecordCount(
        const RumpShahdowTaskState& state) = 0;
    [[nodiscard]] virtual std::uint32_t RecordAt(
        const RumpShahdowTaskState& state, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t MoveLastRecordToFront(
        RumpShahdowTaskState& state) = 0;
    virtual void ClearRecords(RumpShahdowTaskState& state) = 0;
    virtual void SetShowFacet(std::uint32_t facet, bool shown) = 0;
    virtual void DispatchChildEvent(std::uint32_t child, bool retiring) = 0;
    [[nodiscard]] virtual RumpShahdowTransform ReadOwnerTransform(
        std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::uint32_t ChildFrame(std::uint32_t child) = 0;
    [[nodiscard]] virtual bool FrameHasParent(std::uint32_t frame) = 0;
    virtual void RemoveFrameChild(std::uint32_t frame) = 0;
    // Frame operations are supplied through the native RenderWare SDK boundary.
    virtual void FrameSetIdentity(std::uint32_t frame) = 0;
    virtual void FrameApplyTransform(std::uint32_t frame,
                                     const RumpShahdowTransform& transform) = 0;
    virtual void FrameUpdateObjects(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(double value) = 0;
};

class IRumpShahdowConfigRuntime {
public:
    virtual ~IRumpShahdowConfigRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t config,
                                           std::string_view key,
                                           float fallback) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(std::uint32_t config,
                                                std::string_view key,
                                                std::int32_t fallback) = 0;
};

class RumpShahdowTask final {
public:
    static constexpr std::uint32_t kVtable = 0x84ae68u;

    // IDA 0x6220a0/0x622100/0x622170. The stack byte at +96 is explicit.
    static RumpShahdowTaskState& Construct(
        RumpShahdowTaskState& state, std::uint8_t stack_byte,
        IRumpShahdowTaskRuntime& runtime);
    static RumpShahdowTaskState& Destroy(
        RumpShahdowTaskState& state, IRumpShahdowTaskRuntime& runtime);
    [[nodiscard]] static std::string_view ClassName();
    // IDA 0x622180.
    [[nodiscard]] static bool LoadConfig(
        RumpShahdowTaskState& state, std::uint32_t config,
        IRumpShahdowConfigRuntime& runtime);
    // IDA 0x6222b0/0x6225e0/0x622680.
    [[nodiscard]] static std::int32_t InitializeShadows(
        RumpShahdowTaskState& state, IRumpShahdowTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearShadows(
        RumpShahdowTaskState& state, IRumpShahdowTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t ActivateShadows(
        RumpShahdowTaskState& state, IRumpShahdowTaskRuntime& runtime);
    // IDA 0x6226f0/0x622d70.
    [[nodiscard]] static std::uint8_t Update(
        RumpShahdowTaskState& state, float elapsed,
        IRumpShahdowTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(
        RumpShahdowTaskState& state, IRumpShahdowTaskRuntime& runtime);
};

}  // namespace pal4
