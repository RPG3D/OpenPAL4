#include "pal4/collection_slot_operations.h"

namespace pal4 {
namespace {

std::uint32_t Size(const CollectionSlotState& slot, std::uint32_t stride) {
    return slot.begin == 0u ? 0u :
           static_cast<std::uint32_t>(
               static_cast<std::int32_t>(slot.end - slot.begin) >>
               (stride == 4u ? 2 : 3));
}

std::uint32_t Reserve(CollectionSlotState& slot, std::uint32_t count,
                      std::uint32_t stride, ICollectionSlotRuntime& runtime) {
    const auto capacity = slot.begin == 0u ? 0u :
        static_cast<std::uint32_t>(
            static_cast<std::int32_t>(slot.capacity_end - slot.begin) >>
            (stride == 4u ? 2 : 3));
    if (capacity >= count)
        return capacity;

    const auto old_begin = slot.begin;
    const auto old_end = slot.end;
    const auto replacement = runtime.Allocate(count * stride);
    for (auto source = old_begin, target = replacement;
         source != old_end; source += stride, target += stride) {
        if (target != 0u) {
            runtime.WriteDword(target, runtime.ReadDword(source));
            if (stride == 8u)
                runtime.WriteDword(target + 4u,
                                   runtime.ReadDword(source + 4u));
        }
    }
    runtime.Free(old_begin);
    slot.capacity_end = replacement + count * stride;
    slot.begin = replacement;
    slot.end = replacement + (old_begin == 0u ? 0u :
                               static_cast<std::uint32_t>(
                                   static_cast<std::int32_t>(old_end - old_begin) >>
                                   (stride == 4u ? 2 : 3)) * stride);
    return slot.end;
}

std::uint32_t Resize(CollectionSlotState& slot, std::uint32_t count,
                     std::uint32_t stride, std::uint32_t first,
                     std::uint32_t second, ICollectionSlotRuntime& runtime) {
    const auto old_size = Size(slot, stride);
    if (old_size >= count) {
        if (slot.begin != 0u && count < old_size) {
            const auto old_end = slot.end;
            slot.end = slot.begin + count * stride;
            return old_end;
        }
        return old_size;
    }

    const auto extra = count - old_size;
    const auto spare = static_cast<std::uint32_t>(
        static_cast<std::int32_t>(slot.capacity_end - slot.end) >>
        (stride == 4u ? 2 : 3));
    if (spare >= extra) {
        for (auto i = old_size; i < count; ++i) {
            runtime.WriteDword(slot.end + (i - old_size) * stride, first);
            if (stride == 8u)
                runtime.WriteDword(slot.end + (i - old_size) * stride + 4u,
                                   second);
        }
        slot.end += extra * stride;
        return extra * stride;
    }

    const auto capacity = old_size + (old_size > extra ? old_size : extra);
    const auto replacement = runtime.Allocate(capacity * stride);
    for (std::uint32_t offset = 0u; offset < old_size * stride;
         offset += stride) {
        const auto target = replacement + offset;
        if (target != 0u) {
            runtime.WriteDword(target,
                               runtime.ReadDword(slot.begin + offset));
            if (stride == 8u)
                runtime.WriteDword(target + 4u,
                                   runtime.ReadDword(slot.begin + offset + 4u));
        }
    }
    for (auto i = old_size; i < count; ++i) {
        const auto target = replacement + i * stride;
        if (target != 0u) {
            runtime.WriteDword(target, first);
            if (stride == 8u)
                runtime.WriteDword(target + 4u, second);
        }
    }
    if (stride == 8u)
        runtime.BeforeFreePairs(slot.begin, slot.end);
    runtime.Free(slot.begin);
    slot.begin = replacement;
    slot.end = replacement + count * stride;
    slot.capacity_end = replacement + capacity * stride;
    return count;
}

}  // namespace

std::uint32_t CollectionSlotRuntimeStub::Allocate(std::uint32_t) { return 0u; }
std::uint32_t CollectionSlotRuntimeStub::ReadDword(std::uint32_t) { return 0u; }
void CollectionSlotRuntimeStub::WriteDword(std::uint32_t, std::uint32_t) {}
void CollectionSlotRuntimeStub::BeforeFreePairs(
    std::uint32_t, std::uint32_t) {}
void CollectionSlotRuntimeStub::Free(std::uint32_t) {}

std::uint32_t CollectionSlotOperations::CopyPairs(
    CollectionSlotState& slot, std::uint32_t destination,
    std::uint32_t source, ICollectionSlotRuntime& runtime) {
    const auto result = destination;
    while (source != slot.end) {
        runtime.WriteDword(destination, runtime.ReadDword(source));
        runtime.WriteDword(destination + 4u, runtime.ReadDword(source + 4u));
        source += 8u;
        destination += 8u;
    }
    slot.end = destination;
    return result;
}

std::uint32_t CollectionSlotOperations::ReserveWords(
    CollectionSlotState& slot, std::uint32_t count,
    ICollectionSlotRuntime& runtime) {
    return Reserve(slot, count, 4u, runtime);
}

std::uint32_t CollectionSlotOperations::ResizeWords(
    CollectionSlotState& slot, std::uint32_t count, std::uint32_t fill,
    ICollectionSlotRuntime& runtime) {
    return Resize(slot, count, 4u, fill, 0u, runtime);
}

std::uint32_t CollectionSlotOperations::ReservePairs(
    CollectionSlotState& slot, std::uint32_t count,
    ICollectionSlotRuntime& runtime) {
    return Reserve(slot, count, 8u, runtime);
}

std::uint32_t CollectionSlotOperations::ResizePairs(
    CollectionSlotState& slot, std::uint32_t count,
    std::uint32_t first, std::uint32_t second,
    ICollectionSlotRuntime& runtime) {
    return Resize(slot, count, 8u, first, second, runtime);
}

std::uint32_t CollectionRecordArray208::Count(
    const CollectionSlotState& slot) {
    if (slot.begin == 0u)
        return 0u;
    return static_cast<std::uint32_t>(
        static_cast<std::int32_t>(slot.end - slot.begin) / 208);
}

}  // namespace pal4
