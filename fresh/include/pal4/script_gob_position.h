#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptGobPositionRuntime {
public:
    virtual ~IScriptGobPositionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindObject(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        std::uint32_t object, const std::array<float, 3>& position,
        std::uint32_t flags) = 0;
};

class ScriptGobPosition final {
public:
    // IDA 0x5d74c0, giGOBSetPosition.
    [[nodiscard]] static std::uint32_t Set(
        std::uint32_t name_object, float x, float y, float z,
        IScriptGobPositionRuntime& runtime);
};

}  // namespace pal4
