#pragma once

#include <cstdint>

namespace pal4 {

class IScriptCurrentPlayerPositionRuntime {
public:
    virtual ~IScriptCurrentPlayerPositionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
};

class ScriptCurrentPlayerPosition final {
public:
    // IDA 0x5d78c0/0x5d78f0/0x5d7920.
    [[nodiscard]] static double X(
        IScriptCurrentPlayerPositionRuntime& runtime);
    [[nodiscard]] static double Y(
        IScriptCurrentPlayerPositionRuntime& runtime);
    [[nodiscard]] static double Z(
        IScriptCurrentPlayerPositionRuntime& runtime);

private:
    [[nodiscard]] static double Axis(
        std::uint32_t offset,
        IScriptCurrentPlayerPositionRuntime& runtime);
};

}  // namespace pal4
