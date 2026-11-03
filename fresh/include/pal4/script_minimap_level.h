#pragma once

#include <cstdint>

namespace pal4 {

class IScriptMinimapLevelRuntime {
public:
    virtual ~IScriptMinimapLevelRuntime() = default;
    [[nodiscard]] virtual std::uint32_t MinimapManager() = 0;
    virtual void SetTexture(std::uint32_t minimap,
                            std::uint32_t level) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void UpdateGrid(std::uint32_t minimap,
                            std::uint32_t direction,
                            std::uint32_t position) = 0;
};

class ScriptMinimapLevel final {
public:
    // IDA 0x5d8710, giSetMinimapLevel.
    static void Set(std::uint32_t level,
                    IScriptMinimapLevelRuntime& runtime);
};

}  // namespace pal4
