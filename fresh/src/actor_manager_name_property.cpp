#include "pal4/actor_manager_name_property.h"

namespace pal4 {

std::uint32_t ActorManagerNameProperty::Address(std::uint32_t actor) {
    return actor + kOffset;
}

std::uint8_t ActorManagerNameProperty::Assign(
    StringBufferState& destination, const StringBufferState& source,
    IActorManagerNamePropertyRuntime& runtime) {
    const auto length = source.length;
    if (&destination == &source) {
        if (destination.length < length) runtime.ThrowRange();
        (void)runtime.DetachAndReset(destination);
        const auto remaining = destination.length - length;
        std::uint32_t removed = 0xffffffffu;
        if (remaining != 0xffffffffu) {
            removed = remaining;
            if (removed == 0u) return runtime.DetachAndReset(destination);
        }
        runtime.Copy(destination.data + length,
                     destination.data + length + removed,
                     remaining - removed);
        const auto new_length = destination.length - removed;
        if (runtime.AllocateText(destination, new_length, 0u))
            runtime.SetTextSize(destination, new_length);
        return runtime.DetachAndReset(destination);
    }

    const auto source_data = source.data ? source.data : kEmptyText;
    if (length != 0u && runtime.ReadByte(source_data - 1u) < 0xfeu) {
        if (destination.data) {
            const auto reference = runtime.ReadByte(destination.data - 1u);
            if (reference == 0u || reference == 0xffu)
                runtime.Free(destination.data - 1u);
            else
                runtime.WriteByte(destination.data - 1u,
                                  static_cast<std::uint8_t>(reference - 1u));
        }
        destination = {source_data, source.length, source.capacity};
        const auto reference = runtime.ReadByte(source_data - 1u);
        runtime.WriteByte(source_data - 1u,
                          static_cast<std::uint8_t>(reference + 1u));
        return static_cast<std::uint8_t>(source_data);
    }

    if (!runtime.AllocateText(destination, length, 1u)) return 0u;
    runtime.Copy(destination.data, source_data, length);
    destination.length = length;
    runtime.WriteByte(destination.data + length, 0u);
    return static_cast<std::uint8_t>(length);
}

}  // namespace pal4
