#pragma once

#include "pal4/script_enum_object_lifecycle.h"

#include <cstdint>

namespace pal4 {

struct ScriptOwnedAdapterSlot {
    std::uint32_t flag_word = 0;
    std::uint32_t pointer = 0;
};
static_assert(sizeof(ScriptOwnedAdapterSlot) == 8u);

class IScriptBindingCallbackRuntime {
public:
    virtual ~IScriptBindingCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyAdapter(
        std::uint32_t address, std::uint32_t flags) = 0;
};

class ScriptBindingCallbackFactory final {
public:
    // IDA 0x587410.
    [[nodiscard]] static ScriptOwnedAdapterSlot& Assign(
        ScriptOwnedAdapterSlot& destination,
        ScriptOwnedAdapterSlot& source,
        IScriptBindingCallbackRuntime& runtime);
    // IDA 0x587470..0x587e90. The native entry selects vtable and slot.
    [[nodiscard]] static std::uint32_t CreateAndAssign(
        std::uint32_t native_entry, ScriptEnumAdapterPairState& slots,
        std::uint32_t callback, std::uint32_t index,
        IScriptBindingCallbackRuntime& runtime);
    // IDA 0x587f50..0x588370: five earlier and 26 adjacent facet constructors.
    [[nodiscard]] static std::uint32_t ConstructFacet(
        std::uint32_t native_entry, std::uint32_t storage,
        std::uint32_t callback, std::uint32_t index,
        IScriptBindingCallbackRuntime& runtime);
};

}  // namespace pal4
