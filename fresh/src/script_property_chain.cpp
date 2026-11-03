#include "pal4/script_property_chain.h"

namespace pal4 {
namespace {

std::uint8_t Allows(std::uint32_t object,
                    std::uint32_t virtual_offset,
                    std::int32_t first, std::int32_t second,
                    IScriptPropertyChainRuntime& runtime) {
    const auto link = runtime.ReadWord(object + 8u);
    if (link == 0u) return 1u;
    return static_cast<std::uint8_t>(
        runtime.CallLink(link, virtual_offset, first, second));
}

}  // namespace

std::uint8_t ScriptPropertyChain::ReturnZero() { return 0u; }
std::int32_t ScriptPropertyChain::CountTwo() { return 2; }
std::int32_t ScriptPropertyChain::CountEight() { return 8; }

std::uint8_t ScriptPropertyChain::DestroyBase12(
    std::uint32_t object, IScriptPropertyChainRuntime& runtime) {
    runtime.WriteWord(object, 0x845fd0u);
    const auto active = runtime.ReadByte(object + 4u);
    if (active == 0u) return active;
    const auto link = runtime.ReadWord(object + 8u);
    if (link == 0u) return active;
    return static_cast<std::uint8_t>(runtime.CallLink(link, 0u, 1, 0));
}

std::uint8_t ScriptPropertyChain::AllowsAtFour(
    std::uint32_t object, std::int32_t argument,
    IScriptPropertyChainRuntime& runtime) {
    return Allows(object, 4u, argument, 0, runtime);
}

std::uint8_t ScriptPropertyChain::AllowsAtEight(
    std::uint32_t object, std::int32_t first,
    std::int32_t second, IScriptPropertyChainRuntime& runtime) {
    return Allows(object, 8u, first, second, runtime);
}

std::uint8_t ScriptPropertyChain::AllowsAtTwelve(
    std::uint32_t object, std::int32_t first,
    std::int32_t second, IScriptPropertyChainRuntime& runtime) {
    return Allows(object, 12u, first, second, runtime);
}

std::int32_t ScriptPropertyChain::BroadcastValue(
    std::uint32_t object, std::int32_t argument,
    IScriptPropertyChainRuntime& runtime) {
    auto result = runtime.CallValue(object, 344u, argument);
    auto cursor = runtime.ReadWord(object + 8u);
    while (cursor != 0u) {
        result = runtime.CallValue(cursor, 344u, argument);
        cursor = runtime.ReadWord(cursor + 8u);
    }
    return result;
}

void ScriptPropertyChain::UpdateFromCurrent(
    std::uint32_t object, std::int32_t argument,
    IScriptPropertyChainRuntime& runtime) {
    const auto value = runtime.CallValue(object, 16u, 0);
    PropagateByValue(object, argument, value, runtime);
}

std::uint8_t ScriptPropertyChain::AssignLinkFromSlot(
    std::uint32_t object, std::uint32_t source_slot,
    IScriptPropertyChainRuntime& runtime) {
    const auto link = runtime.ReadWord(source_slot + 4u);
    const auto destination = object + 4u;
    if (destination == source_slot) {
        const auto active = runtime.ReadByte(source_slot);
        if (active != 0u && link != 0u)
            return static_cast<std::uint8_t>(
                runtime.CallLink(link, 0u, 1, 0));
        return active;
    }

    const auto old_link = runtime.ReadWord(object + 8u);
    if (old_link == link) {
        if (runtime.ReadByte(source_slot) != 0u)
            runtime.WriteByte(destination, 1u);
    } else {
        if (runtime.ReadByte(destination) != 0u && old_link != 0u) {
            try {
                (void)runtime.CallLink(old_link, 0u, 1, 0);
            } catch (...) {
                if (runtime.ReadByte(source_slot) != 0u && link != 0u)
                    (void)runtime.CallLink(link, 0u, 1, 0);
                throw;
            }
        }
        const auto source_active = runtime.ReadByte(source_slot);
        runtime.WriteWord(object + 8u, link);
        runtime.WriteByte(destination, source_active);
        return 0u;
    }
    runtime.WriteWord(object + 8u, link);
    return 0u;
}

void ScriptPropertyChain::PropagateByValue(
    std::uint32_t object, std::int32_t argument,
    std::int32_t value, IScriptPropertyChainRuntime& runtime) {
    auto cursor = object;
    while (cursor != 0u) {
        const auto next = runtime.ReadWord(cursor + 8u);
        if (next != 0u &&
            runtime.CallValue(next, 16u, 0) == value) {
            (void)DetachMatchingNext(cursor, argument, runtime);
        } else {
            cursor = runtime.ReadWord(cursor + 8u);
        }
    }
}

std::uint32_t ScriptPropertyChain::DetachMatchingNext(
    std::uint32_t object, std::int32_t argument,
    IScriptPropertyChainRuntime& runtime) {
    const auto was_active = runtime.ReadByte(object + 4u);
    const auto next = runtime.ReadWord(object + 8u);
    runtime.WriteByte(object + 4u, 0u);

    std::uint32_t result = 0u;
    if (next != object) {
        const auto successor = runtime.ReadWord(next + 8u);
        if (next == successor) {
            if (runtime.ReadByte(next + 4u) != 0u)
                runtime.WriteByte(object + 4u, 1u);
        } else {
            runtime.WriteByte(object + 4u,
                              runtime.ReadByte(next + 4u));
        }
        runtime.WriteByte(next + 4u, 0u);
        runtime.WriteWord(object + 8u, runtime.ReadWord(next + 8u));
    }

    if (next != 0u) {
        try {
            result = runtime.CallLink(next, 24u, argument, 0);
        } catch (...) {
            if (was_active != 0u)
                (void)runtime.CallLink(next, 0u, 1, 0);
            throw;
        }
    }
    if (was_active != 0u && next != 0u)
        result = runtime.CallLink(next, 0u, 1, 0);
    return result;
}

}  // namespace pal4
