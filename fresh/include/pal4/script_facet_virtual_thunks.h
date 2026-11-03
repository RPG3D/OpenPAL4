#pragma once

#include <cstdint>
#include <span>

namespace pal4 {

class IScriptFacetVirtualRuntime {
public:
    virtual ~IScriptFacetVirtualRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Invoke(
        std::uint32_t object, std::uint32_t vtable_offset,
        std::span<const std::uint32_t> arguments) = 0;
};

class ScriptFacetVirtualThunks final {
public:
    // IDA 0x588c00..0x588e90. A tail jump forwards the original call frame.
    [[nodiscard]] static std::uint32_t Dispatch(
        std::uint32_t native_entry, std::uint32_t object,
        std::span<const std::uint32_t> arguments,
        IScriptFacetVirtualRuntime& runtime);
};

}  // namespace pal4
