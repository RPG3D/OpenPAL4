#include "pal4/script_binding_callback_factory.h"

#include <array>
#include <stdexcept>

namespace pal4 {
namespace {

struct FactoryEntry {
    std::uint32_t address;
    std::uint32_t vtable;
    std::uint32_t size;
    std::uint32_t slot;
};

constexpr std::array<FactoryEntry, 15> kFactories{{
    {0x587470, 0x8482c0, 8, 1}, {0x587530, 0x8482c8, 8, 0},
    {0x5875e0, 0x8482d0, 8, 1}, {0x5876a0, 0x8482d8, 8, 0},
    {0x587750, 0x8482e0, 8, 1}, {0x587810, 0x8482e8, 8, 0},
    {0x5878c0, 0x8482f0, 8, 1}, {0x587980, 0x8482f8, 8, 0},
    {0x587a30, 0x848300, 8, 1}, {0x587af0, 0x848308, 8, 0},
    {0x587ba0, 0x848310, 8, 1}, {0x587c60, 0x848318, 8, 0},
    {0x587d10, 0x848320, 8, 1}, {0x587dd0, 0x848328, 12, 0},
    {0x587e90, 0x848330, 12, 1},
}};

struct FacetEntry {
    std::uint32_t address;
    std::uint32_t vtable;
    bool indexed;
};

constexpr std::array<FacetEntry, 31> kFacets{{
    {0x587f50, 0x848338, false},
    {0x587f70, 0x848340, false},
    {0x587f90, 0x848348, false},
    {0x587fc0, 0x848350, false},
    {0x587fe0, 0x848358, true},
    {0x588010, 0x848360, true},
    {0x588030, 0x848368, true},
    {0x588050, 0x848370, true},
    {0x588070, 0x848378, true},
    {0x588090, 0x848380, true},
    {0x5880b0, 0x848388, true},
    {0x5880d0, 0x848390, true},
    {0x5880f0, 0x848398, true},
    {0x588110, 0x8483a0, true},
    {0x588130, 0x8483a8, true},
    {0x588150, 0x8483b0, true},
    {0x588190, 0x8483b8, true},
    {0x5881b0, 0x8483c0, true},
    {0x5881d0, 0x8483c8, true},
    {0x5881f0, 0x8483d0, true},
    {0x588210, 0x8483d8, true},
    {0x588230, 0x8483e0, true},
    {0x588250, 0x8483e8, true},
    {0x588270, 0x8483f0, true},
    {0x5882b0, 0x8483f8, false},
    {0x5882d0, 0x848400, false},
    {0x5882f0, 0x848408, false},
    {0x588310, 0x848410, false},
    {0x588330, 0x848418, false},
    {0x588350, 0x848420, false},
    {0x588370, 0x848428, false},
}};

ScriptOwnedAdapterSlot ReadSlot(const ScriptEnumAdapterPairState& slots,
                                std::uint32_t index) {
    return {slots.words[index * 2u], slots.words[index * 2u + 1u]};
}

void WriteSlot(ScriptEnumAdapterPairState& slots,
               std::uint32_t index, const ScriptOwnedAdapterSlot& slot) {
    slots.words[index * 2u] = slot.flag_word;
    slots.words[index * 2u + 1u] = slot.pointer;
}

}  // namespace

ScriptOwnedAdapterSlot& ScriptBindingCallbackFactory::Assign(
    ScriptOwnedAdapterSlot& destination, ScriptOwnedAdapterSlot& source,
    IScriptBindingCallbackRuntime& runtime) {
    if (&destination == &source) return destination;
    if (destination.pointer == source.pointer) {
        if (static_cast<std::uint8_t>(source.flag_word) != 0u)
            destination.flag_word = (destination.flag_word & 0xffffff00u) | 1u;
    } else {
        if (static_cast<std::uint8_t>(destination.flag_word) != 0u &&
            destination.pointer != 0u)
            static_cast<void>(runtime.DestroyAdapter(destination.pointer, 1u));
        destination.flag_word =
            (destination.flag_word & 0xffffff00u) |
            static_cast<std::uint8_t>(source.flag_word);
    }
    destination.pointer = source.pointer;
    source.flag_word &= 0xffffff00u;
    return destination;
}

std::uint32_t ScriptBindingCallbackFactory::CreateAndAssign(
    std::uint32_t native_entry, ScriptEnumAdapterPairState& slots,
    std::uint32_t callback, std::uint32_t index,
    IScriptBindingCallbackRuntime& runtime) {
    for (const auto& entry : kFactories) {
        if (entry.address != native_entry) continue;
        const auto created = runtime.Allocate(entry.size);
        if (created != 0u) {
            runtime.WriteWord(created, entry.vtable);
            runtime.WriteWord(created + 4u, callback);
            if (entry.size == 12u)
                runtime.WriteWord(created + 8u, index);
        }

        auto destination = ReadSlot(slots, entry.slot);
        std::uint32_t result = created;
        if (destination.pointer == created) {
            if (created != 0u)
                destination.flag_word =
                    (destination.flag_word & 0xffffff00u) | 1u;
        } else {
            if (static_cast<std::uint8_t>(destination.flag_word) != 0u &&
                destination.pointer != 0u)
                result = runtime.DestroyAdapter(destination.pointer, 1u);
            destination.flag_word =
                (destination.flag_word & 0xffffff00u) |
                static_cast<std::uint32_t>(created != 0u);
        }
        destination.pointer = created;
        WriteSlot(slots, entry.slot, destination);
        return result;
    }
    throw std::invalid_argument("unknown callback factory entry");
}

std::uint32_t ScriptBindingCallbackFactory::ConstructFacet(
    std::uint32_t native_entry, std::uint32_t storage,
    std::uint32_t callback, std::uint32_t index,
    IScriptBindingCallbackRuntime& runtime) {
    for (const auto& entry : kFacets) {
        if (entry.address != native_entry) continue;
        runtime.WriteWord(storage, entry.vtable);
        runtime.WriteWord(storage + 4u, callback);
        if (entry.indexed) runtime.WriteWord(storage + 8u, index);
        return storage;
    }
    throw std::invalid_argument("unknown facet constructor entry");
}

}  // namespace pal4
