#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptMonsterVisibilityRuntime {
public:
    virtual ~IScriptMonsterVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t MonsterManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindMonster(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t FindMonsterInfo(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::string_view AnimationName(
        std::uint32_t animation) = 0;
    [[nodiscard]] virtual std::int32_t CompareIgnoreCase(
        std::string_view lhs, std::string_view rhs) = 0;
    [[nodiscard]] virtual std::uint32_t PlayAnimation(
        std::uint32_t animation, std::string_view name) = 0;
    virtual void SetStateFlag(std::uint32_t monster,
                              std::uint32_t state) = 0;
    virtual void ReplaceComponent(std::uint32_t monster,
                                  std::uint32_t component,
                                  std::uint32_t enabled) = 0;
    virtual void CleanupBuffers(std::uint32_t monster,
                                std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t EnterCharacter(
        std::uint32_t arena, std::uint32_t monster) = 0;
    [[nodiscard]] virtual std::uint32_t LeaveCharacter(
        std::uint32_t arena, std::uint32_t monster) = 0;
};

class ScriptMonsterVisibility final {
public:
    // IDA 0x5d5640, giMonsterSetHide.
    [[nodiscard]] static std::uint32_t SetHide(
        std::uint32_t name_payload, std::uint8_t hidden,
        IScriptMonsterVisibilityRuntime& runtime);
    // IDA 0x5d6060, giMonsterSetVisible.
    [[nodiscard]] static std::uint32_t SetVisible(
        std::uint32_t name_payload, std::uint8_t visible,
        IScriptMonsterVisibilityRuntime& runtime);
};

}  // namespace pal4
