#include "pal4/game_object_animation_event_array.h"

#include <algorithm>
#include <limits>

namespace pal4 {

void GameObjectAnimationEventArray::CleanupRange(
    std::uint32_t begin, const std::uint32_t end,
    IGameObjectAnimationEventArrayRuntime& runtime) {
    if (end < begin || (end - begin) % kEventStride != 0) return;
    for (; begin != end; begin += kEventStride)
        runtime.DestroyEvent(begin);
}

int GameObjectAnimationEventArray::Release(
    GameObjectAnimationEventArrayState& state,
    IGameObjectAnimationEventInsertRuntime& runtime) {
    CleanupRange(state.begin, state.end, runtime);
    runtime.Free(state.begin);
    state.begin = state.end = state.capacity = 0;
    return 0;
}

std::uint32_t GameObjectAnimationEventArray::CopyRangeForward(
    std::uint32_t source_begin, const std::uint32_t source_end,
    std::uint32_t destination_begin,
    IGameObjectAnimationEventCopyRuntime& runtime) {
    if (source_end < source_begin ||
        (source_end - source_begin) % kEventStride != 0 ||
        destination_begin > std::numeric_limits<std::uint32_t>::max() -
                                (source_end - source_begin))
        return destination_begin;
    for (; source_begin != source_end;
         source_begin += kEventStride, destination_begin += kEventStride)
        runtime.AssignEvent(destination_begin, source_begin);
    return destination_begin;
}

void GameObjectAnimationEventArray::FillRange(
    std::uint32_t destination_begin, const std::uint32_t destination_end,
    const std::uint32_t source_event,
    IGameObjectAnimationEventCopyRuntime& runtime) {
    if (destination_end < destination_begin ||
        (destination_end - destination_begin) % kEventStride != 0)
        return;
    for (; destination_begin != destination_end;
         destination_begin += kEventStride)
        runtime.AssignEvent(destination_begin, source_event);
}

std::uint32_t GameObjectAnimationEventArray::CopyRangeBackward(
    const std::uint32_t source_begin, std::uint32_t source_end,
    std::uint32_t destination_end,
    IGameObjectAnimationEventCopyRuntime& runtime) {
    if (source_end < source_begin ||
        (source_end - source_begin) % kEventStride != 0 ||
        destination_end < source_end - source_begin)
        return destination_end;
    while (source_end != source_begin) {
        source_end -= kEventStride;
        destination_end -= kEventStride;
        runtime.AssignEvent(destination_end, source_end);
    }
    return destination_end;
}

std::uint32_t GameObjectAnimationEventArray::Insert(
    GameObjectAnimationEventArrayState& state,
    const std::uint32_t position, const std::uint32_t count,
    const std::uint32_t source_event,
    IGameObjectAnimationEventInsertRuntime& runtime) {
    const auto begin = state.begin;
    const auto end = state.end;
    const auto capacity = state.capacity;
    if ((begin == 0 && (end != 0 || capacity != 0 || position != 0)) ||
        (begin != 0 &&
         (begin > position || position > end || end > capacity ||
          (position - begin) % kEventStride != 0 ||
          (end - begin) % kEventStride != 0 ||
          (capacity - begin) % kEventStride != 0)))
        return 0;

    const auto size = begin == 0 ? 0u : (end - begin) / kEventStride;
    const auto spare = begin == 0 ? 0u : (capacity - end) / kEventStride;
    if (count == 0) return 0;
    if (count > std::numeric_limits<std::uint32_t>::max() / kEventStride - size)
        return 0;
    const auto bytes = count * kEventStride;

    if (spare >= count) {
        const auto tail = (end - position) / kEventStride;
        if (tail >= count) {
            for (std::uint32_t index = 0; index < count; ++index)
                runtime.CopyRaw(end + index * kEventStride,
                                end - bytes + index * kEventStride);
            (void)CopyRangeBackward(position, end - bytes, end, runtime);
            FillRange(position, position + bytes, source_event, runtime);
        } else {
            for (std::uint32_t index = 0; index < tail; ++index)
                runtime.CopyRaw(position + bytes + index * kEventStride,
                                position + index * kEventStride);
            for (std::uint32_t index = 0; index < count - tail; ++index)
                runtime.CopyRaw(end + index * kEventStride, source_event);
            FillRange(position, end, source_event, runtime);
        }
        state.end = end + bytes;
        return state.end;
    }

    const auto new_capacity_count = size + std::max(size, count);
    if (new_capacity_count >
        std::numeric_limits<std::uint32_t>::max() / kEventStride)
        return 0;
    const auto allocation = runtime.Allocate(new_capacity_count * kEventStride);
    if (allocation == 0) return 0;
    if (allocation > std::numeric_limits<std::uint32_t>::max() -
                         new_capacity_count * kEventStride) {
        runtime.Free(allocation);
        return 0;
    }

    std::uint32_t destination = allocation;
    for (auto source = begin; source != position; source += kEventStride) {
        runtime.CopyRaw(destination, source);
        destination += kEventStride;
    }
    for (std::uint32_t index = 0; index < count; ++index) {
        runtime.CopyRaw(destination, source_event);
        destination += kEventStride;
    }
    for (auto source = position; source != end; source += kEventStride) {
        runtime.CopyRaw(destination, source);
        destination += kEventStride;
    }
    CleanupRange(begin, end, runtime);
    runtime.Free(begin);
    state.capacity = allocation + new_capacity_count * kEventStride;
    state.begin = allocation;
    state.end = destination;
    return 41u * (size + count);
}

}  // namespace pal4
