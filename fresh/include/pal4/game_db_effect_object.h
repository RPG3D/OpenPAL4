#pragma once

#include "pal4/animation_track.h"
#include "pal4/game_db_typed_value.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct GameDbEffectObjectState final {
    std::uint32_t vtable = 0u;
    GameDbValueArrayState index;
    GameDbValueArrayState records;
    GameDbValueArrayState tracks;
    GameDbStringObjectState name;
    GameDbStringObjectState metadata;
    std::uint32_t key_index = 0u;
    std::uint32_t lock_flags = 0u;
};
static_assert(sizeof(GameDbEffectObjectState) == 92u);
static_assert(offsetof(GameDbEffectObjectState, index) == 4u);
static_assert(offsetof(GameDbEffectObjectState, records) == 20u);
static_assert(offsetof(GameDbEffectObjectState, tracks) == 36u);
static_assert(offsetof(GameDbEffectObjectState, name) == 52u);
static_assert(offsetof(GameDbEffectObjectState, metadata) == 68u);
static_assert(offsetof(GameDbEffectObjectState, key_index) == 84u);

class IGameDbEffectObjectRuntime {
public:
    virtual ~IGameDbEffectObjectRuntime() = default;
    [[nodiscard]] virtual bool AllocateText(GameDbStringObjectState& text,
                                            std::uint32_t length) = 0;
    virtual void WriteText(std::uint32_t destination, const char* source,
                           std::uint32_t length) = 0;
    virtual void ClearEffectData(GameDbEffectObjectState& object) = 0;
    virtual void ReleaseText(std::uint32_t data) = 0;
    virtual void CleanupTrack(std::uint32_t address) = 0;
    virtual void CleanupRecord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

struct GameDbEffectIndexPair final {
    std::int32_t key = 0;
    std::uint32_t track_index = 0u;
};
static_assert(sizeof(GameDbEffectIndexPair) == 8u);

class IGameDbEffectAddRuntime {
public:
    virtual ~IGameDbEffectAddRuntime() = default;
    [[nodiscard]] virtual std::int32_t GetRowKey(
        std::uint32_t row, std::uint32_t property_index) = 0;
    virtual void AppendTrack(GameDbValueArrayState& tracks,
                             std::uint32_t row) = 0;
    virtual void AssignTrack(std::uint32_t track_address,
                             std::uint32_t row) = 0;
    [[nodiscard]] virtual GameDbEffectIndexPair ReadPair(
        std::uint32_t address) = 0;
    virtual void WritePair(std::uint32_t address,
                           GameDbEffectIndexPair pair) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateIndex(
        std::uint32_t bytes) = 0;
    virtual void FreeIndex(std::uint32_t address) = 0;
};

class IGameDbEffectAssignRuntime {
public:
    virtual ~IGameDbEffectAssignRuntime() = default;
    virtual void ClearEffectData(GameDbEffectObjectState& object) = 0;
    virtual void AssignRecord(std::uint32_t destination,
                              std::uint32_t source) = 0;
    virtual void ConstructRecord(std::uint32_t destination,
                                 std::uint32_t source) = 0;
    virtual void DestroyRecord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateRecords(
        std::uint32_t bytes) = 0;
    virtual void FreeRecords(std::uint32_t address) = 0;
    virtual void Finalize(GameDbEffectObjectState& object,
                          std::uint32_t value) = 0;
};

class IGameDbEffectClearRuntime {
public:
    virtual ~IGameDbEffectClearRuntime() = default;
    virtual void ClearTrack(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ClearRecord(
        std::uint32_t address) = 0;
};

class IGameDbEffectTrackRuntime {
public:
    virtual ~IGameDbEffectTrackRuntime() = default;
    virtual void ConstructTrack(std::uint32_t destination,
                                std::uint32_t source) = 0;
    virtual void FillTrack(std::uint32_t destination,
                           std::uint32_t event) = 0;
    virtual void AssignEvents(std::uint32_t destination,
                              std::uint32_t event) = 0;
    virtual void CleanupTrack(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateTracks(
        std::uint32_t bytes) = 0;
    virtual void FreeTracks(std::uint32_t address) = 0;
};

class IGameDbEffectEnableRuntime {
public:
    virtual ~IGameDbEffectEnableRuntime() = default;
    [[nodiscard]] virtual std::int32_t CallVirtual24(
        GameDbEffectObjectState& object) = 0;
};

class IGameDbEffectSortRuntime : public IAnimationTrackDistinctRuntime {
public:
    virtual ~IGameDbEffectSortRuntime() = default;
    virtual void CleanupTrack(std::uint32_t address) = 0;
    [[nodiscard]] virtual GameDbEffectIndexPair ReadPair(
        std::uint32_t address) = 0;
    virtual void WritePair(std::uint32_t address,
                           GameDbEffectIndexPair pair) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateIndex(
        std::uint32_t bytes) = 0;
    virtual void FreeIndex(std::uint32_t address) = 0;
};

class IGameDbEffectResizeRuntime {
public:
    virtual ~IGameDbEffectResizeRuntime() = default;
    [[nodiscard]] virtual GameDbEffectIndexPair ReadPair(
        std::uint32_t address) = 0;
    virtual void WritePair(std::uint32_t address,
                           GameDbEffectIndexPair pair) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CopyConstructTrack(std::uint32_t destination,
                                    std::uint32_t source) = 0;
    virtual void CleanupTrack(std::uint32_t address) = 0;
};

class GameDbEffectObject final {
public:
    static constexpr std::uint32_t kVtable = 0x84b4e0u;

    // IDA 0x6560f0: initialize the 92-byte effect and copy its name.
    [[nodiscard]] static GameDbEffectObjectState& Construct(
        GameDbEffectObjectState& object, const char* name,
        IGameDbEffectObjectRuntime& runtime);
    // IDA 0x656200: release text and each owned array in native order.
    [[nodiscard]] static std::uint32_t Destroy(
        GameDbEffectObjectState& object,
        IGameDbEffectObjectRuntime& runtime);
    // IDA 0x656320: replace a keyed track or append it and insert a sorted key.
    [[nodiscard]] static bool AddRow(
        GameDbEffectObjectState& object, std::uint32_t row,
        IGameDbEffectAddRuntime& runtime);
    // IDA 0x6567d0: replace 60-byte schema records and finalize the effect.
    [[nodiscard]] static bool AssignSchema(
        GameDbEffectObjectState& object,
        const GameDbValueArrayState& source,
        std::uint32_t key_index, std::uint32_t finalize_value,
        IGameDbEffectAssignRuntime& runtime);
    // IDA 0x656ce0: clear live tracks, index and records, retaining capacity.
    [[nodiscard]] static std::uint32_t ClearData(
        GameDbEffectObjectState& object,
        IGameDbEffectClearRuntime& runtime);
    // IDA 0x656dc0: map a sorted index slot to its 16-byte track address.
    [[nodiscard]] static std::uint32_t TrackAt(
        const GameDbEffectObjectState& object, std::uint32_t index,
        IGameDbEffectAddRuntime& runtime);
    // IDA 0x656e00: append a track only in the locked array mode.
    [[nodiscard]] static std::uint8_t AddLockedTrack(
        GameDbEffectObjectState& object, std::uint32_t event,
        IGameDbEffectTrackRuntime& runtime);
    // IDA 0x656fc0: clear lock byte and forward through virtual slot +24.
    [[nodiscard]] static std::int32_t Enable(
        GameDbEffectObjectState& object,
        IGameDbEffectEnableRuntime& runtime);
    // IDA 0x656fd0: compact consecutive equal-key tracks and rebuild the index.
    static void ProcessAndSortEvents(
        GameDbEffectObjectState& object,
        IGameDbEffectSortRuntime& runtime);
    // IDA 0x657210: reserve both index and animation-track arrays.
    [[nodiscard]] static std::uint32_t ResizeArrays(
        GameDbEffectObjectState& object, std::uint32_t count,
        IGameDbEffectResizeRuntime& runtime);
};

}  // namespace pal4
