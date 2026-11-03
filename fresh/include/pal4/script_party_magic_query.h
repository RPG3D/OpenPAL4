#pragma once

#include <cstdint>

namespace pal4 {

class IScriptPartyMagicQueryRuntime {
public:
    virtual ~IScriptPartyMagicQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool HasAnyValues(std::uint32_t collection) = 0;
};

class ScriptPartyMagicQuery final {
public:
    // IDA 0x5d6e50, giCheckMagicMastered.
    [[nodiscard]] static bool AnyMastered(IScriptPartyMagicQueryRuntime& runtime);
};

}  // namespace pal4
