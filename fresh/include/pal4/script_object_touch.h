#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptObjectTouchRuntime {
public:
    virtual ~IScriptObjectTouchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindByName(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t Touch(
        std::uint32_t object, std::uint32_t player_direction) = 0;
};

class ScriptObjectTouch final {
public:
    // IDA 0x5d4d50, giGOMTouch.
    [[nodiscard]] static std::uint32_t Run(
        const char* object_name, IScriptObjectTouchRuntime& runtime);
};

}  // namespace pal4
