#pragma once

#include <cstdint>

namespace pal4 {

class IScriptVisibilityQueryRuntime {
public:
    virtual ~IScriptVisibilityQueryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t MonsterManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindObject(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t FindMonster(
        std::uint32_t manager, std::uint32_t name) = 0;
};

class ScriptVisibilityQuery final {
public:
    // IDA 0x5d6c70, giGetVisibleObject.
    [[nodiscard]] static bool Object(std::uint32_t name_payload,
                                     IScriptVisibilityQueryRuntime& runtime);
    // IDA 0x5d6cb0, giGetVisibleMonster.
    [[nodiscard]] static bool Monster(std::uint32_t name_payload,
                                      IScriptVisibilityQueryRuntime& runtime);
};

}  // namespace pal4
