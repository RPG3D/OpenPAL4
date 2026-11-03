#include "pal4/animation_script_record_array.h"

#include <algorithm>
#include <limits>

namespace pal4 {

namespace {

std::uint32_t CleanupRecords(const AnimationScriptRecordArrayState& state,
                             IAnimationScriptRecordRuntime& runtime) {
    if (state.end < state.begin ||
        (state.end - state.begin) % AnimationScriptRecordArray::kRecordStride != 0)
        return 0;
    std::uint32_t result = 0;
    for (auto address = state.begin; address != state.end;
         address += AnimationScriptRecordArray::kRecordStride) {
        result = AnimationScriptRecord::Cleanup(runtime.RecordAt(address), runtime);
    }
    return result;
}

}  // namespace

std::int32_t AnimationScriptRecordArray::Count(
    const AnimationScriptRecordArrayState& state) {
    return state.begin != 0
        ? static_cast<std::int32_t>(state.end - state.begin) >> 5 : 0;
}

std::uint32_t AnimationScriptRecordArray::AddressAt(
    const AnimationScriptRecordArrayState& state,
    const std::uint32_t index) {
    return state.begin + kRecordStride * index;
}

int AnimationScriptRecordArray::Release(
    AnimationScriptRecordArrayState& state,
    IAnimationScriptRecordArrayRuntime& runtime) {
    const auto begin = state.begin;
    const auto end = state.end;
    if (end >= begin && (end - begin) % kRecordStride == 0) {
        for (auto record = begin; record != end; record += kRecordStride) {
            runtime.SetErrorFlag(record + 16u, 1u);
            runtime.SetErrorFlag(record, 1u);
        }
    }
    state.end = begin;
    runtime.Free(begin);
    state.begin = 0;
    state.end = 0;
    state.capacity = 0;
    return 0;
}

std::uint32_t AnimationScriptRecordArray::ClearRecords(
    AnimationScriptRecordArrayState& state,
    IAnimationScriptRecordRuntime& runtime) {
    const auto result = CleanupRecords(state, runtime);
    state.end = state.begin;
    return result;
}

int AnimationScriptRecordArray::ReleaseData(
    AnimationScriptRecordArrayState& state,
    IAnimationScriptRecordRuntime& runtime) {
    (void)CleanupRecords(state, runtime);
    (void)runtime.Free(state.begin);
    state.begin = 0;
    state.end = 0;
    state.capacity = 0;
    return 0;
}

void AnimationScriptRecordArray::Insert(
    AnimationScriptRecordArrayState& state,
    const std::uint32_t position, const std::uint32_t count,
    const std::uint32_t source_record,
    IAnimationScriptRecordInsertRuntime& runtime) {
    const auto begin = state.begin;
    const auto end = state.end;
    const auto capacity = state.capacity;
    if ((begin == 0 && (end != 0 || capacity != 0 || position != 0)) ||
        (begin != 0 &&
         (begin > position || position > end || end > capacity ||
          (position - begin) % kRecordStride != 0 ||
          (end - begin) % kRecordStride != 0 ||
          (capacity - begin) % kRecordStride != 0)))
        return;

    const auto size = begin == 0 ? 0u : (end - begin) / kRecordStride;
    const auto spare = begin == 0 ? 0u : (capacity - end) / kRecordStride;
    if (count == 0 ||
        count > std::numeric_limits<std::uint32_t>::max() / kRecordStride - size)
        return;
    const auto bytes = count * kRecordStride;

    if (spare >= count) {
        const auto tail = (end - position) / kRecordStride;
        if (tail >= count) {
            for (std::uint32_t index = 0; index < count; ++index)
                runtime.CopyRecord(end + index * kRecordStride,
                                   end - bytes + index * kRecordStride);
            for (auto source = end - bytes; source != position;) {
                source -= kRecordStride;
                runtime.AssignString(source + bytes, source);
                runtime.AssignString(source + bytes + 16u, source + 16u);
            }
            for (auto destination = position;
                 destination != position + bytes;
                 destination += kRecordStride) {
                runtime.AssignString(destination, source_record);
                runtime.AssignString(destination + 16u, source_record + 16u);
            }
        } else {
            for (std::uint32_t index = 0; index < tail; ++index)
                runtime.CopyRecord(position + bytes + index * kRecordStride,
                                   position + index * kRecordStride);
            for (std::uint32_t index = 0; index < count - tail; ++index)
                runtime.CopyRecord(end + index * kRecordStride, source_record);
            for (auto destination = position; destination != end;
                 destination += kRecordStride) {
                runtime.AssignString(destination, source_record);
                runtime.AssignString(destination + 16u, source_record + 16u);
            }
        }
        state.end = end + bytes;
        return;
    }

    const auto capacity_count = size + std::max(size, count);
    if (capacity_count >
        std::numeric_limits<std::uint32_t>::max() / kRecordStride)
        return;
    const auto allocation = runtime.Allocate(capacity_count * kRecordStride);
    if (allocation == 0) return;
    if (allocation > std::numeric_limits<std::uint32_t>::max() -
                         capacity_count * kRecordStride) {
        (void)runtime.Free(allocation);
        return;
    }

    auto destination = allocation;
    for (auto source = begin; source != position;
         source += kRecordStride, destination += kRecordStride)
        runtime.CopyRecord(destination, source);
    for (std::uint32_t index = 0; index < count;
         ++index, destination += kRecordStride)
        runtime.CopyRecord(destination, source_record);
    for (auto source = position; source != end;
         source += kRecordStride, destination += kRecordStride)
        runtime.CopyRecord(destination, source);

    (void)CleanupRecords(state, runtime);
    (void)runtime.Free(begin);
    state.begin = allocation;
    state.end = destination;
    state.capacity = allocation + capacity_count * kRecordStride;
}

}  // namespace pal4
