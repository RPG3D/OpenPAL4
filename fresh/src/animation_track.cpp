#include "pal4/animation_track.h"

namespace pal4 {

AnimationTrackState& AnimationTrack::Assign(
    AnimationTrackState& destination, const AnimationTrackState& source,
    IAnimationTrackRuntime& runtime) {
    if (&destination == &source)
        return destination;

    const auto source_count = source.begin == 0u
                                  ? 0u : (source.end - source.begin) / 8u;
    const auto old_count = destination.begin == 0u
                               ? 0u : (destination.end - destination.begin) / 8u;
    if (source_count <= old_count) {
        for (auto i = 0u; i < source_count; ++i)
            runtime.SetProperty(destination.begin + i * 8u,
                                source.begin + i * 8u);
        for (auto i = source_count; i < old_count; ++i)
            runtime.DestroyProperty(destination.begin + i * 8u);
        destination.end = destination.begin + source_count * 8u;
        return destination;
    }

    const auto capacity_count = destination.begin == 0u
                                    ? 0u : (destination.capacity - destination.begin) / 8u;
    if (source_count <= capacity_count) {
        for (auto i = 0u; i < old_count; ++i)
            runtime.SetProperty(destination.begin + i * 8u,
                                source.begin + i * 8u);
        for (auto i = old_count; i < source_count; ++i)
            runtime.ConstructProperty(destination.begin + i * 8u,
                                      source.begin + i * 8u);
        destination.end = destination.begin + source_count * 8u;
        return destination;
    }

    for (auto i = 0u; i < old_count; ++i)
        runtime.CleanupProperty(destination.begin + i * 8u);
    runtime.Free(destination.begin);
    const auto memory = runtime.Allocate(source_count * 8u);
    destination.begin = memory;
    for (auto i = 0u; i < source_count; ++i)
        runtime.ConstructProperty(memory + i * 8u,
                                  source.begin + i * 8u);
    destination.end = destination.capacity = memory + source_count * 8u;
    return destination;
}

std::uint32_t AnimationTrack::Clear(
    AnimationTrackState& track, IAnimationTrackClearRuntime& runtime) {
    for (auto address = track.begin; address != track.end; address += 16u)
        runtime.CleanupProperties(address);
    runtime.Free(track.begin);
    track.begin = track.end = track.capacity = 0u;
    return 0u;
}

std::uint32_t AnimationTrack::CleanupRange(
    const std::uint32_t begin, const std::uint32_t end,
    IAnimationTrackRangeRuntime& runtime) {
    std::uint32_t result = 0u;
    for (auto address = begin; address != end; address += 16u)
        result = runtime.CleanupProperties(address);
    return result;
}

std::uint32_t AnimationTrack::CopyRange(
    const std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t destination, IAnimationTrackRangeRuntime& runtime) {
    auto target = destination;
    for (auto source = begin; source != end; source += 16u) {
        runtime.CopyConstruct(target, source);
        target += 16u;
    }
    return target;
}

void AnimationTrack::Insert(
    AnimationTrackState& array, const std::uint32_t position,
    const std::uint32_t count, const std::uint32_t value,
    IAnimationTrackInsertRuntime& runtime) {
    const auto bytes = count * 16u;
    if ((array.capacity - array.end) / 16u >= count) {
        const auto tail = (array.end - position) / 16u;
        if (tail < count) {
            for (auto source = position; source != array.end; source += 16u)
                runtime.CopyConstruct(source + bytes, source);
            for (auto slot = array.end;
                 slot != array.end + (count - tail) * 16u; slot += 16u)
                runtime.CopyConstruct(slot, value);
            for (auto slot = position; slot != array.end; slot += 16u)
                runtime.Assign(slot, value);
        } else if (count != 0u) {
            for (auto source = array.end - bytes;
                 source != array.end; source += 16u)
                runtime.CopyConstruct(source + bytes, source);
            for (auto target = array.end - bytes; target != position;) {
                target -= 16u;
                runtime.Assign(target + bytes, target);
            }
            for (auto target = position; target != position + bytes;
                 target += 16u)
                runtime.Assign(target, value);
        }
        array.end += bytes;
        return;
    }

    const auto old_begin = array.begin;
    const auto old_end = array.end;
    const auto old_count = old_begin == 0u
                               ? 0u : (old_end - old_begin) / 16u;
    const auto new_count = old_count + (old_count > count ? old_count : count);
    const auto memory = runtime.Allocate(new_count * 16u);
    auto target = memory;
    for (auto source = old_begin; source != position; source += 16u) {
        runtime.CopyConstruct(target, source);
        target += 16u;
    }
    for (auto i = 0u; i < count; ++i) {
        runtime.CopyConstruct(target, value);
        target += 16u;
    }
    for (auto source = position; source != old_end; source += 16u) {
        runtime.CopyConstruct(target, source);
        target += 16u;
    }
    for (auto source = old_begin; source != old_end; source += 16u)
        runtime.CleanupProperties(source);
    runtime.Free(old_begin);
    array.begin = memory;
    array.end = memory + (old_count + count) * 16u;
    array.capacity = memory + new_count * 16u;
}

bool AnimationTrack::CompareEvents(
    const std::uint32_t property_index, const std::uint32_t first,
    const std::uint32_t second, IAnimationTrackKeyRuntime& runtime) {
    const auto value = runtime.GetPropertyValue(first, property_index);
    return value == runtime.GetPropertyValue(second, property_index);
}

void AnimationTrack::Fill(
    std::uint32_t destination, const std::uint32_t count,
    const std::uint32_t value, IAnimationTrackRangeRuntime& runtime) {
    for (auto i = 0u; i < count; ++i) {
        runtime.CopyConstruct(destination, value);
        destination += 16u;
    }
}

std::uint32_t AnimationTrack::AssignEvents(
    const std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t source, IAnimationTrackBatchRuntime& runtime) {
    for (auto address = begin; address != end; address += 16u) {
        if (address != source)
            (void)Assign(runtime.TrackAt(address), runtime.TrackAt(source),
                         runtime.PropertyRuntime());
    }
    return end;
}

std::uint32_t AnimationTrack::MoveEvents(
    const std::uint32_t begin, const std::uint32_t end,
    std::uint32_t destination_end, IAnimationTrackBatchRuntime& runtime) {
    for (auto source = end; source != begin;) {
        source -= 16u;
        destination_end -= 16u;
        if (destination_end != source)
            (void)Assign(runtime.TrackAt(destination_end),
                         runtime.TrackAt(source), runtime.PropertyRuntime());
    }
    return destination_end;
}

void AnimationTrack::CopyConstruct(
    AnimationTrackState* destination, const AnimationTrackState& source,
    IAnimationTrackRuntime& runtime) {
    if (destination == nullptr)
        return;
    destination->flag = (destination->flag & 0xffffff00u) |
                        (source.flag & 0xffu);
    const auto count = source.begin == 0u
                           ? 0u : (source.end - source.begin) / 8u;
    const auto memory = runtime.Allocate(count * 8u);
    destination->begin = memory;
    auto target = memory;
    for (auto property = source.begin; property != source.end;
         property += 8u) {
        if (target != 0u)
            runtime.ConstructProperty(target, property);
        target += 8u;
    }
    destination->end = destination->capacity = target;
}

std::uint32_t AnimationTrackProperty::Destroy(
    const std::uint32_t address, const std::uint8_t flags,
    IAnimationTrackPropertyDestroyRuntime& runtime) {
    runtime.CleanupProperty(address);
    if ((flags & 1u) != 0u)
        runtime.Free(address);
    return address;
}

std::uint32_t AnimationTrack::DestroyAt(
    const std::uint32_t address, const std::uint8_t flags,
    IAnimationTrackClearRuntime& runtime) {
    runtime.CleanupProperties(address);
    if ((flags & 1u) != 0u)
        runtime.Free(address);
    return address;
}

std::uint32_t AnimationTrack::CopyDistinctRuns(
    const std::uint32_t begin, const std::uint32_t end,
    std::uint32_t destination, const std::uint32_t property_index,
    IAnimationTrackDistinctRuntime& runtime) {
    if (begin == end)
        return destination;
    auto previous = begin;
    runtime.AssignTrack(destination, begin);
    destination += 16u;
    for (auto source = begin + 16u; source != end; source += 16u) {
        if (runtime.GetTrackKey(previous, property_index) ==
            runtime.GetTrackKey(source, property_index))
            continue;
        previous = source;
        if (destination != source)
            runtime.CopyTrackProperties(destination, source);
        destination += 16u;
    }
    return destination;
}

}  // namespace pal4
