#pragma once

#include <cstdint>

namespace pal4 {

class IScriptPlayerFavorRuntime {
public:
    virtual ~IScriptPlayerFavorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class ScriptPlayerFavor final {
public:
    // IDA 0x5d8670, giAddPlayerFavor.
    [[nodiscard]] static std::uint32_t Add(
        std::int32_t first_index, std::int32_t second_index,
        std::int32_t delta, IScriptPlayerFavorRuntime& runtime);
};

}  // namespace pal4
