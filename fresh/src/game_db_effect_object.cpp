#include "pal4/game_db_effect_object.h"

#include <cstring>
#include <vector>

namespace pal4 {

namespace {

void InitializeFlaggedField(std::uint32_t& field,
                            const std::uint8_t source_byte) {
    field = (field & 0xffffff00u) | source_byte;
}

void ResetPointers(GameDbValueArrayState& array) {
    array.begin = array.end = array.capacity = 0u;
}

void ReleaseText(GameDbStringObjectState& text,
                 IGameDbEffectObjectRuntime& runtime) {
    if (text.data != 0u)
        runtime.ReleaseText(text.data);
    text.data = text.length = text.capacity = 0u;
}

}  // namespace

GameDbEffectObjectState& GameDbEffectObject::Construct(
    GameDbEffectObjectState& object, const char* name,
    IGameDbEffectObjectRuntime& runtime) {
    const auto source_byte = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(name));
    InitializeFlaggedField(object.index.flag, source_byte);
    ResetPointers(object.index);
    InitializeFlaggedField(object.records.flag, source_byte);
    ResetPointers(object.records);
    InitializeFlaggedField(object.tracks.flag, source_byte);
    ResetPointers(object.tracks);
    InitializeFlaggedField(object.name.flag, source_byte);
    object.name.data = object.name.length = object.name.capacity = 0u;
    const auto length = static_cast<std::uint32_t>(std::strlen(name));
    if (runtime.AllocateText(object.name, length)) {
        runtime.WriteText(object.name.data, name, length);
        object.name.length = length;
    }
    InitializeFlaggedField(object.metadata.flag, source_byte);
    object.metadata.data = object.metadata.length =
        object.metadata.capacity = 0u;
    object.key_index = 0u;
    object.lock_flags &= 0xffffff00u;
    object.vtable = kVtable;
    return object;
}

std::uint32_t GameDbEffectObject::Destroy(
    GameDbEffectObjectState& object,
    IGameDbEffectObjectRuntime& runtime) {
    object.vtable = kVtable;
    runtime.ClearEffectData(object);
    ReleaseText(object.metadata, runtime);
    ReleaseText(object.name, runtime);
    for (auto address = object.tracks.begin; address != object.tracks.end;
         address += 16u)
        runtime.CleanupTrack(address);
    (void)runtime.Free(object.tracks.begin);
    ResetPointers(object.tracks);
    for (auto address = object.records.begin; address != object.records.end;
         address += 60u)
        runtime.CleanupRecord(address);
    (void)runtime.Free(object.records.begin);
    ResetPointers(object.records);
    const auto result = runtime.Free(object.index.begin);
    ResetPointers(object.index);
    return result;
}

bool GameDbEffectObject::AddRow(
    GameDbEffectObjectState& object, const std::uint32_t row,
    IGameDbEffectAddRuntime& runtime) {
    if ((object.lock_flags & 0xffu) != 0u)
        return false;
    const auto key = runtime.GetRowKey(row, object.key_index);
    const auto track_index = object.tracks.begin == 0u
                                 ? 0u : (object.tracks.end - object.tracks.begin) / 16u;
    auto first = object.index.begin;
    auto remaining = (object.index.end - first) / 8u;
    while (remaining != 0u) {
        const auto half = remaining / 2u;
        const auto middle = first + half * 8u;
        if (runtime.ReadPair(middle).key < key) {
            first = middle + 8u;
            remaining -= half + 1u;
        } else {
            remaining = half;
        }
    }
    if (first != object.index.end) {
        const auto existing = runtime.ReadPair(first);
        if (existing.key == key) {
            runtime.AssignTrack(object.tracks.begin + 16u * existing.track_index,
                                row);
            return true;
        }
    }

    runtime.AppendTrack(object.tracks, row);
    const GameDbEffectIndexPair inserted{key, track_index};
    const auto old_begin = object.index.begin;
    const auto old_end = object.index.end;
    if ((object.index.capacity - old_end) / 8u != 0u) {
        for (auto source = old_end; source != first;) {
            source -= 8u;
            runtime.WritePair(source + 8u, runtime.ReadPair(source));
        }
        runtime.WritePair(first, inserted);
        object.index.end += 8u;
        return true;
    }

    const auto old_count = old_begin == 0u
                               ? 0u : (old_end - old_begin) / 8u;
    const auto growth = old_count > 1u ? old_count : 1u;
    const auto new_count = old_count + growth;
    const auto memory = runtime.AllocateIndex(new_count * 8u);
    auto destination = memory;
    for (auto source = old_begin; source != first; source += 8u) {
        runtime.WritePair(destination, runtime.ReadPair(source));
        destination += 8u;
    }
    runtime.WritePair(destination, inserted);
    destination += 8u;
    for (auto source = first; source != old_end; source += 8u) {
        runtime.WritePair(destination, runtime.ReadPair(source));
        destination += 8u;
    }
    runtime.FreeIndex(old_begin);
    object.index.begin = memory;
    object.index.end = memory + (old_count + 1u) * 8u;
    object.index.capacity = memory + new_count * 8u;
    return true;
}

bool GameDbEffectObject::AssignSchema(
    GameDbEffectObjectState& object,
    const GameDbValueArrayState& source,
    const std::uint32_t key_index,
    const std::uint32_t finalize_value,
    IGameDbEffectAssignRuntime& runtime) {
    runtime.ClearEffectData(object);
    auto& records = object.records;
    if (&records != &source) {
        const auto source_count = source.begin == 0u
                                      ? 0u : (source.end - source.begin) / 60u;
        const auto old_count = records.begin == 0u
                                   ? 0u : (records.end - records.begin) / 60u;
        if (source_count <= old_count) {
            for (auto i = 0u; i < source_count; ++i)
                runtime.AssignRecord(records.begin + i * 60u,
                                     source.begin + i * 60u);
            for (auto i = source_count; i < old_count; ++i)
                runtime.DestroyRecord(records.begin + i * 60u);
            records.end = records.begin + source_count * 60u;
        } else {
            const auto capacity_count = records.begin == 0u
                                            ? 0u : (records.capacity - records.begin) / 60u;
            if (source_count > capacity_count) {
                for (auto i = 0u; i < old_count; ++i)
                    runtime.DestroyRecord(records.begin + i * 60u);
                runtime.FreeRecords(records.begin);
                const auto memory = runtime.AllocateRecords(source_count * 60u);
                records.begin = memory;
                for (auto i = 0u; i < source_count; ++i)
                    runtime.ConstructRecord(memory + i * 60u,
                                            source.begin + i * 60u);
                records.end = records.capacity = memory + source_count * 60u;
            } else {
                for (auto i = 0u; i < old_count; ++i)
                    runtime.AssignRecord(records.begin + i * 60u,
                                         source.begin + i * 60u);
                for (auto i = old_count; i < source_count; ++i)
                    runtime.ConstructRecord(records.begin + i * 60u,
                                            source.begin + i * 60u);
                records.end = records.begin + source_count * 60u;
            }
        }
    }
    object.key_index = key_index;
    runtime.Finalize(object, finalize_value);
    return true;
}

std::uint32_t GameDbEffectObject::ClearData(
    GameDbEffectObjectState& object,
    IGameDbEffectClearRuntime& runtime) {
    for (auto address = object.tracks.begin; address != object.tracks.end;
         address += 16u)
        runtime.ClearTrack(address);
    object.tracks.end = object.tracks.begin;
    auto result = object.index.end;
    object.index.end = object.index.begin;
    for (auto address = object.records.begin; address != object.records.end;
         address += 60u)
        result = runtime.ClearRecord(address);
    object.records.end = object.records.begin;
    return result;
}

std::uint32_t GameDbEffectObject::TrackAt(
    const GameDbEffectObjectState& object, const std::uint32_t index,
    IGameDbEffectAddRuntime& runtime) {
    if ((object.lock_flags & 0xffu) != 0u || object.index.begin == 0u ||
        index >= (object.index.end - object.index.begin) / 8u)
        return 0u;
    const auto pair = runtime.ReadPair(object.index.begin + index * 8u);
    return object.tracks.begin + pair.track_index * 16u;
}

std::uint8_t GameDbEffectObject::AddLockedTrack(
    GameDbEffectObjectState& object, const std::uint32_t event,
    IGameDbEffectTrackRuntime& runtime) {
    const auto lock = static_cast<std::uint8_t>(object.lock_flags);
    if (lock == 0u)
        return 0u;
    auto& tracks = object.tracks;
    if ((tracks.capacity - tracks.end) / 16u != 0u) {
        runtime.FillTrack(tracks.end, event);
        runtime.AssignEvents(tracks.end, event);
        tracks.end += 16u;
        return static_cast<std::uint8_t>(tracks.end);
    }

    const auto old_begin = tracks.begin;
    const auto old_end = tracks.end;
    const auto old_count = old_begin == 0u
                               ? 0u : (old_end - old_begin) / 16u;
    const auto growth = old_count > 1u ? old_count : 1u;
    const auto new_count = old_count + growth;
    const auto memory = runtime.AllocateTracks(new_count * 16u);
    for (auto index = 0u; index < old_count; ++index)
        runtime.ConstructTrack(memory + index * 16u,
                               old_begin + index * 16u);
    runtime.FillTrack(memory + old_count * 16u, event);
    for (auto index = 0u; index < old_count; ++index)
        runtime.CleanupTrack(old_begin + index * 16u);
    runtime.FreeTracks(old_begin);
    tracks.begin = memory;
    tracks.end = memory + (old_count + 1u) * 16u;
    tracks.capacity = memory + new_count * 16u;
    return static_cast<std::uint8_t>(old_begin);
}

std::int32_t GameDbEffectObject::Enable(
    GameDbEffectObjectState& object,
    IGameDbEffectEnableRuntime& runtime) {
    object.lock_flags &= 0xffffff00u;
    return runtime.CallVirtual24(object);
}

void GameDbEffectObject::ProcessAndSortEvents(
    GameDbEffectObjectState& object,
    IGameDbEffectSortRuntime& runtime) {
    auto& tracks = object.tracks;
    auto compact = tracks.end;
    for (auto current = tracks.begin + 16u; current < tracks.end;
         current += 16u) {
        if (runtime.GetTrackKey(current - 16u, object.key_index) ==
            runtime.GetTrackKey(current, object.key_index)) {
            compact = current - 16u;
            break;
        }
    }
    if (compact != tracks.end) {
        const auto destination = AnimationTrack::CopyDistinctRuns(
            compact, tracks.end, compact, object.key_index, runtime);
        for (auto address = destination; address < tracks.end;
             address += 16u)
            runtime.CleanupTrack(address);
        tracks.end = destination;
    }

    auto& index = object.index;
    index.end = index.begin;
    const auto track_count = tracks.begin == 0u
                                 ? 0u : (tracks.end - tracks.begin) / 16u;
    for (auto ordinal = 0u; ordinal < track_count; ++ordinal) {
        const GameDbEffectIndexPair pair{
            runtime.GetTrackKey(tracks.begin + ordinal * 16u,
                                object.key_index), ordinal};
        if (index.end == index.capacity) {
            const auto old_count = index.begin == 0u
                                       ? 0u : (index.end - index.begin) / 8u;
            const auto new_count = old_count + (old_count > 1u ? old_count : 1u);
            const auto memory = runtime.AllocateIndex(new_count * 8u);
            for (auto i = 0u; i < old_count; ++i)
                runtime.WritePair(memory + i * 8u,
                                  runtime.ReadPair(index.begin + i * 8u));
            runtime.FreeIndex(index.begin);
            index.begin = memory;
            index.end = memory + old_count * 8u;
            index.capacity = memory + new_count * 8u;
        }
        runtime.WritePair(index.end, pair);
        index.end += 8u;
    }

    // The native small-range insertion sort compares signed keys only.
    for (auto address = index.begin + 8u; address < index.end;
         address += 8u) {
        const auto pair = runtime.ReadPair(address);
        auto slot = address;
        while (slot > index.begin &&
               pair.key < runtime.ReadPair(slot - 8u).key) {
            runtime.WritePair(slot, runtime.ReadPair(slot - 8u));
            slot -= 8u;
        }
        runtime.WritePair(slot, pair);
    }
}

std::uint32_t GameDbEffectObject::ResizeArrays(
    GameDbEffectObjectState& object, const std::uint32_t count,
    IGameDbEffectResizeRuntime& runtime) {
    auto& index = object.index;
    const auto index_capacity = index.begin == 0u
                                    ? 0u : (index.capacity - index.begin) / 8u;
    if (index_capacity < count) {
        const auto old_begin = index.begin;
        const auto old_count = old_begin == 0u
                                   ? 0u : (index.end - old_begin) / 8u;
        const auto memory = runtime.Allocate(
            8u * (count & 0x80000000u ? 0u : count));
        for (auto i = 0u; i < old_count; ++i)
            runtime.WritePair(memory + i * 8u,
                              runtime.ReadPair(old_begin + i * 8u));
        runtime.Free(old_begin);
        index.begin = memory;
        index.end = memory + old_count * 8u;
        index.capacity = memory + count * 8u;
    }

    auto& tracks = object.tracks;
    const auto track_capacity = tracks.begin == 0u
                                    ? 0u : (tracks.capacity - tracks.begin) / 16u;
    if (track_capacity >= count)
        return track_capacity;

    const auto old_begin = tracks.begin;
    const auto old_count = old_begin == 0u
                               ? 0u : (tracks.end - old_begin) / 16u;
    const auto memory = runtime.Allocate(
        16u * (count & 0x80000000u ? 0u : count));
    for (auto i = 0u; i < old_count; ++i)
        runtime.CopyConstructTrack(memory + i * 16u,
                                   old_begin + i * 16u);
    for (auto i = 0u; i < old_count; ++i)
        runtime.CleanupTrack(old_begin + i * 16u);
    runtime.Free(old_begin);
    tracks.begin = memory;
    tracks.end = memory + old_count * 16u;
    tracks.capacity = memory + count * 16u;
    return tracks.end;
}

}  // namespace pal4
