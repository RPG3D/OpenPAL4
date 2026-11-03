#include "pal4/actor_manager_property_queries.h"

#include <initializer_list>

namespace pal4 {

namespace {

std::uint32_t CopyTextVariantAt(
    const std::uint32_t actor, const std::uint32_t destination,
    const std::int32_t selector, const std::uint32_t first_offset,
    IActorManagerPropertyQueryRuntime& runtime) {
    if (selector == 0 || selector == 1) {
        const auto source = actor + first_offset +
                            static_cast<std::uint32_t>(selector) * 32u;
        runtime.WriteByte(destination, runtime.ReadByte(source));
        for (const auto offset : {4u, 8u, 12u})
            runtime.WriteWord(destination + offset, 0u);
        runtime.AssignText(destination, source, 0u, 0xffffffffu);
        return destination;
    }

    runtime.WriteByte(destination,
                      static_cast<std::uint8_t>(destination));
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(destination + offset, 0u);
    const auto length = runtime.StringLength(
        ActorManagerPropertyQueries::kEmptyErrorString);
    if (runtime.AllocateText(destination, length, 1u)) {
        const auto buffer = runtime.ReadWord(destination + 4u);
        runtime.Copy(buffer, ActorManagerPropertyQueries::kEmptyErrorString,
                     length);
        runtime.WriteWord(destination + 8u, length);
        runtime.WriteByte(buffer + length, 0u);
    }
    return destination;
}

}  // namespace

std::uint32_t ActorManagerPropertyQueries::FixedDescriptor() {
    return kFixedDescriptor;
}

std::uint32_t ActorManagerPropertyQueries::StateDescriptor(
    const std::uint32_t actor, IActorManagerPropertyQueryRuntime& runtime) {
    return StateDescriptor(runtime.ActorStateSlot20(actor));
}

std::uint32_t ActorManagerPropertyQueries::StateDescriptor(const std::int32_t state) {
    if (state >= 6) return kDefaultDescriptor;
    return kDescriptorTable + static_cast<std::uint32_t>(state) * 12u;
}

std::uint32_t ActorManagerPropertyQueries::NameOrEmpty(
    const std::uint32_t actor, IActorManagerPropertyQueryRuntime& runtime) {
    const auto text = runtime.ReadWord(actor + 988u);
    return text ? text : kEmptyText;
}

std::uint32_t ActorManagerPropertyQueries::CopyTextVariant(
    const std::uint32_t actor, const std::uint32_t destination,
    const std::int32_t selector,
    IActorManagerPropertyQueryRuntime& runtime) {
    return CopyTextVariantAt(actor, destination, selector, 1064u, runtime);
}

std::uint32_t ActorManagerPlayerPropertyQueries::StateDescriptor(
    const std::uint32_t actor,
    IActorManagerPropertyQueryRuntime& runtime) {
    return StateDescriptor(runtime.ActorStateSlot20(actor));
}

std::uint32_t ActorManagerPlayerPropertyQueries::StateDescriptor(const std::int32_t state) {
    if (state >= 3) return kDefaultDescriptor;
    return kDescriptorTable + static_cast<std::uint32_t>(state) * 12u;
}

std::uint32_t ActorManagerPlayerPropertyQueries::FixedDescriptor() {
    return kFixedDescriptor;
}

std::uint32_t ActorManagerPlayerPropertyQueries::CopyTextVariant(
    const std::uint32_t actor, const std::uint32_t destination,
    const std::int32_t selector,
    IActorManagerPropertyQueryRuntime& runtime) {
    return CopyTextVariantAt(actor, destination, selector, 1208u, runtime);
}

}  // namespace pal4
