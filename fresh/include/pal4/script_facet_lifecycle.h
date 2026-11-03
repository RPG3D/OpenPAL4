#pragma once

#include <cstdint>

namespace pal4 {

class IScriptFacetLifecycleRuntime {
public:
    virtual ~IScriptFacetLifecycleRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ScriptFacetLifecycle final {
public:
    // IDA 0x5886f0..0x588ba0: the 26 non-deleting destructors.
    static void RestoreVtable(std::uint32_t native_entry,
                              std::uint32_t object,
                              IScriptFacetLifecycleRuntime& runtime);

    // IDA 0x5883b0..0x5886d0 and 0x588700..0x588bb0.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t native_entry, std::uint32_t object,
        std::uint32_t flags, IScriptFacetLifecycleRuntime& runtime);
};

}  // namespace pal4
