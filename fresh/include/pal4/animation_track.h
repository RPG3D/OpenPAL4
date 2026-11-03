#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct AnimationTrackState final {
    std::uint32_t flag = 0u;
    std::uint32_t begin = 0u;
    std::uint32_t end = 0u;
    std::uint32_t capacity = 0u;
};
static_assert(sizeof(AnimationTrackState) == 16u);
static_assert(offsetof(AnimationTrackState, begin) == 4u);

class IAnimationTrackRuntime {
public:
    virtual ~IAnimationTrackRuntime() = default;
    virtual void SetProperty(std::uint32_t destination,
                             std::uint32_t source) = 0;
    virtual void ConstructProperty(std::uint32_t destination,
                                   std::uint32_t source) = 0;
    virtual void DestroyProperty(std::uint32_t address) = 0;
    virtual void CleanupProperty(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IAnimationTrackClearRuntime {
public:
    virtual ~IAnimationTrackClearRuntime() = default;
    virtual void CleanupProperties(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IAnimationTrackRangeRuntime {
public:
    virtual ~IAnimationTrackRangeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CleanupProperties(
        std::uint32_t address) = 0;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source) = 0;
};

class IAnimationTrackInsertRuntime {
public:
    virtual ~IAnimationTrackInsertRuntime() = default;
    virtual void CopyConstruct(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void Assign(std::uint32_t destination,
                        std::uint32_t source) = 0;
    virtual void CleanupProperties(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IAnimationTrackKeyRuntime {
public:
    virtual ~IAnimationTrackKeyRuntime() = default;
    [[nodiscard]] virtual std::int32_t GetPropertyValue(
        std::uint32_t track, std::uint32_t property_index) = 0;
};

class IAnimationTrackBatchRuntime {
public:
    virtual ~IAnimationTrackBatchRuntime() = default;
    [[nodiscard]] virtual AnimationTrackState& TrackAt(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual IAnimationTrackRuntime& PropertyRuntime() = 0;
};

class IAnimationTrackPropertyDestroyRuntime {
public:
    virtual ~IAnimationTrackPropertyDestroyRuntime() = default;
    virtual void CleanupProperty(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class IAnimationTrackDistinctRuntime {
public:
    virtual ~IAnimationTrackDistinctRuntime() = default;
    [[nodiscard]] virtual std::int32_t GetTrackKey(
        std::uint32_t track, std::uint32_t property_index) = 0;
    virtual void AssignTrack(std::uint32_t destination,
                             std::uint32_t source) = 0;
    virtual void CopyTrackProperties(std::uint32_t destination,
                                     std::uint32_t source) = 0;
};

class AnimationTrackProperty final {
public:
    // IDA 0x657d80: cleanup a tagged property and optionally free its object.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t address, std::uint8_t flags,
        IAnimationTrackPropertyDestroyRuntime& runtime);
};

class AnimationTrack final {
public:
    // IDA 0x657360: assign the owned array of 8-byte tagged properties.
    [[nodiscard]] static AnimationTrackState& Assign(
        AnimationTrackState& destination,
        const AnimationTrackState& source,
        IAnimationTrackRuntime& runtime);
    // IDA 0x657550: release 16-byte property blocks and clear array pointers.
    [[nodiscard]] static std::uint32_t Clear(
        AnimationTrackState& track, IAnimationTrackClearRuntime& runtime);
    // IDA 0x657590: cleanup every 16-byte track in a half-open range.
    [[nodiscard]] static std::uint32_t CleanupRange(
        std::uint32_t begin, std::uint32_t end,
        IAnimationTrackRangeRuntime& runtime);
    // IDA 0x6575c0: copy-construct a half-open track range.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, IAnimationTrackRangeRuntime& runtime);
    // IDA 0x657600: insert repeated owned tracks at an array slot.
    static void Insert(AnimationTrackState& array, std::uint32_t position,
                       std::uint32_t count, std::uint32_t value,
                       IAnimationTrackInsertRuntime& runtime);
    // IDA 0x657880: compare the selected property value of two tracks.
    [[nodiscard]] static bool CompareEvents(
        std::uint32_t property_index, std::uint32_t first,
        std::uint32_t second, IAnimationTrackKeyRuntime& runtime);
    // IDA 0x6578c0: copy-construct the same track into consecutive slots.
    static void Fill(std::uint32_t destination, std::uint32_t count,
                     std::uint32_t value,
                     IAnimationTrackRangeRuntime& runtime);
    // IDA 0x6578f0: assign one source track's events across a range.
    [[nodiscard]] static std::uint32_t AssignEvents(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t source, IAnimationTrackBatchRuntime& runtime);
    // IDA 0x657ad0: assign a track range backward into a target ending slot.
    [[nodiscard]] static std::uint32_t MoveEvents(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination_end,
        IAnimationTrackBatchRuntime& runtime);
    // IDA 0x657cc0: initialize a track from a source track.
    static void CopyConstruct(AnimationTrackState* destination,
                              const AnimationTrackState& source,
                              IAnimationTrackRuntime& runtime);
    // IDA 0x657da0: cleanup a track and optionally free its object.
    [[nodiscard]] static std::uint32_t DestroyAt(
        std::uint32_t address, std::uint8_t flags,
        IAnimationTrackClearRuntime& runtime);
    // IDA 0x657dc0: compact consecutive equal-key tracks into a target range.
    [[nodiscard]] static std::uint32_t CopyDistinctRuns(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t destination, std::uint32_t property_index,
        IAnimationTrackDistinctRuntime& runtime);
};

}  // namespace pal4
