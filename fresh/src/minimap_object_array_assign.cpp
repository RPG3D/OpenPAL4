#include "pal4/minimap_object_collection.h"

#include <bit>

namespace pal4 {

std::uint32_t MinimapObjectCollection::AssignNestedArray(
    const std::uint32_t destination,
    const std::uint32_t source,
    IMinimapObjectArrayRuntime& runtime) {
    if (destination == source) return destination;

    const auto source_begin = runtime.ReadDword(source + 4u);
    const auto source_end = runtime.ReadDword(source + 8u);
    const auto source_count = source_begin == 0u ? 0u :
        (source_end - source_begin) / 4u;
    const auto target_begin = runtime.ReadDword(destination + 4u);
    const auto target_end = runtime.ReadDword(destination + 8u);
    const auto target_count = target_begin == 0u ? 0u :
        (target_end - target_begin) / 4u;

    if (source_count <= target_count) {
        for (std::uint32_t index = 0u; index < source_count; ++index) {
            runtime.WriteDword(target_begin + index * 4u,
                               runtime.ReadDword(source_begin + index * 4u));
        }
        runtime.WriteDword(destination + 8u,
                           target_begin + source_count * 4u);
        return destination;
    }

    const auto target_capacity = target_begin == 0u ? 0u :
        (runtime.ReadDword(destination + 12u) - target_begin) / 4u;
    if (source_count > target_capacity) {
        runtime.Free(target_begin);
        const auto signed_count =
            std::bit_cast<std::int32_t>(source_end - source_begin) >> 2;
        const auto allocation_count = signed_count < 0 ? 0u :
            static_cast<std::uint32_t>(signed_count);
        const auto replacement = runtime.Allocate(allocation_count * 4u);
        runtime.WriteDword(destination + 4u, replacement);
        auto target = replacement;
        for (auto pointer = source_begin; pointer != source_end;
             pointer += 4u, target += 4u) {
            if (target != 0u) {
                runtime.WriteDword(target, runtime.ReadDword(pointer));
            }
        }
        runtime.WriteDword(destination + 12u, target);
        runtime.WriteDword(destination + 8u, target);
        return destination;
    }

    for (std::uint32_t index = 0u; index < target_count; ++index) {
        runtime.WriteDword(target_begin + index * 4u,
                           runtime.ReadDword(source_begin + index * 4u));
    }
    for (std::uint32_t index = target_count; index < source_count;
         ++index) {
        runtime.WriteDword(target_end + (index - target_count) * 4u,
                           runtime.ReadDword(source_begin + index * 4u));
    }
    runtime.WriteDword(destination + 8u,
                       target_begin + source_count * 4u);
    return destination;
}

}  // namespace pal4
