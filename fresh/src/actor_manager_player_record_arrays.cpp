#include "pal4/actor_manager_player_record_arrays.h"

namespace pal4 {
namespace {

void CopyRecords(const std::uint32_t destination,
                 const std::uint32_t source, const std::uint32_t count,
                 const std::uint32_t stride,
                 IActorManagerPlayerRecordArrayRuntime& runtime) {
    for (std::uint32_t record = 0u; record < count; ++record) {
        for (std::uint32_t offset = 0u; offset < stride; offset += 4u)
            runtime.WriteWord(destination + record * stride + offset,
                              runtime.ReadWord(source + record * stride +
                                               offset));
    }
}

std::uint32_t Assign(const std::uint32_t destination,
                     const std::uint32_t source, const std::uint32_t stride,
                     IActorManagerPlayerRecordArrayRuntime& runtime) {
    if (destination == source) return destination;

    const auto source_begin = runtime.ReadWord(source + 4u);
    const auto source_end = runtime.ReadWord(source + 8u);
    const auto source_count = source_begin == 0u ? 0u :
                              (source_end - source_begin) / stride;
    const auto old_begin = runtime.ReadWord(destination + 4u);
    const auto old_end = runtime.ReadWord(destination + 8u);
    const auto old_count = old_begin == 0u ? 0u :
                           (old_end - old_begin) / stride;

    if (source_count <= old_count) {
        CopyRecords(old_begin, source_begin, source_count, stride, runtime);
        runtime.WriteWord(destination + 8u,
                          old_begin + source_count * stride);
        return destination;
    }

    const auto old_capacity_end = runtime.ReadWord(destination + 12u);
    const auto old_capacity = old_begin == 0u ? 0u :
                              (old_capacity_end - old_begin) / stride;
    if (source_count <= old_capacity) {
        CopyRecords(old_begin, source_begin, old_count, stride, runtime);
        CopyRecords(old_end, source_begin + old_count * stride,
                    source_count - old_count, stride, runtime);
        runtime.WriteWord(destination + 8u,
                          old_begin + source_count * stride);
        return destination;
    }

    runtime.Free(old_begin);
    const auto replacement = runtime.Allocate(source_count * stride);
    runtime.WriteWord(destination + 4u, replacement);
    if (replacement == 0u) {
        runtime.WriteWord(destination + 8u, 0u);
        runtime.WriteWord(destination + 12u, 0u);
        return destination;
    }
    CopyRecords(replacement, source_begin, source_count, stride, runtime);
    const auto new_end = replacement + source_count * stride;
    runtime.WriteWord(destination + 12u, new_end);
    runtime.WriteWord(destination + 8u, new_end);
    return destination;
}

}  // namespace

std::uint32_t ActorManagerPlayerRecordArrays::AssignPairs(
    const std::uint32_t destination, const std::uint32_t source,
    IActorManagerPlayerRecordArrayRuntime& runtime) {
    return Assign(destination, source, 8u, runtime);
}

std::uint32_t ActorManagerPlayerRecordArrays::AssignTriples(
    const std::uint32_t destination, const std::uint32_t source,
    IActorManagerPlayerRecordArrayRuntime& runtime) {
    return Assign(destination, source, 12u, runtime);
}

}  // namespace pal4
