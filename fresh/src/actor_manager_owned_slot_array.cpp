#include "pal4/actor_manager_owned_slot_array.h"

#include <new>

namespace pal4 {

std::uint32_t ActorManagerOwnedSlotArray::InsertCopies(
    std::uint32_t array, std::uint32_t position, std::uint32_t count,
    std::uint32_t source, IActorManagerOwnedSlotArrayRuntime& runtime) {
    constexpr std::uint32_t stride = 8u;
    const auto begin = runtime.ReadWord(array + 4u);
    const auto end = runtime.ReadWord(array + 8u);
    const auto capacity = runtime.ReadWord(array + 12u);
    const auto bytes = count * stride;

    if ((capacity - end) / stride >= count) {
        if (count == 0u) return 0u;
        const auto tail = (end - position) / stride;
        if (tail >= count) {
            for (auto from = end - bytes; from != end; from += stride)
                (void)ActorManagerOwnedSlot::MoveConstructNullable(
                    from + bytes, from, runtime);
            for (auto from = end - bytes; from != position;) {
                from -= stride;
                (void)ActorManagerOwnedSlot::MoveAssign(
                    from + bytes, from, runtime);
            }
            for (auto destination = position; destination != position + bytes;
                 destination += stride)
                (void)ActorManagerOwnedSlot::MoveAssign(
                    destination, source, runtime);
        } else {
            for (auto from = position; from != end; from += stride)
                (void)ActorManagerOwnedSlot::MoveConstruct(
                    from + bytes, from, runtime);
            for (auto destination = end;
                 destination != end + (count - tail) * stride;
                 destination += stride)
                (void)ActorManagerOwnedSlot::MoveConstruct(
                    destination, source, runtime);
            for (auto destination = position; destination != end;
                 destination += stride)
                (void)ActorManagerOwnedSlot::MoveAssign(
                    destination, source, runtime);
        }
        runtime.WriteWord(array + 8u, end + bytes);
        return bytes;
    }

    const auto used = begin == 0u ? 0u : (end - begin) / stride;
    const auto growth = used > count ? used : count;
    const auto replacement = runtime.Allocate((used + growth) * stride);
    if (replacement == 0u) throw std::bad_alloc();
    auto destination = replacement;
    for (auto from = begin; from != position; from += stride) {
        (void)ActorManagerOwnedSlot::MoveConstructNullable(
            destination, from, runtime);
        destination += stride;
    }
    for (std::uint32_t index = 0u; index != count; ++index) {
        (void)ActorManagerOwnedSlot::MoveConstructNullable(
            destination, source, runtime);
        destination += stride;
    }
    for (auto from = position; from != end; from += stride) {
        (void)ActorManagerOwnedSlot::MoveConstructNullable(
            destination, from, runtime);
        destination += stride;
    }
    for (auto slot = begin; slot != end; slot += stride)
        (void)ActorManagerOwnedSlot::Destroy(slot, 0u, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12u, replacement + (used + growth) * stride);
    runtime.WriteWord(array + 4u, replacement);
    runtime.WriteWord(array + 8u, replacement + (used + count) * stride);
    return replacement + (used + count) * stride;
}

}  // namespace pal4
