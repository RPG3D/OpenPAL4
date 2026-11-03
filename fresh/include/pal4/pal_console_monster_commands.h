#pragma once

#include <cstdint>

namespace pal4 {

class IPalConsoleMonsterRuntime {
public:
    using RemoveCallback = std::uint32_t (*)(
        std::uint32_t monster, IPalConsoleMonsterRuntime& runtime);

    virtual ~IPalConsoleMonsterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void LeaveCharacter(std::uint32_t arena,
                                std::uint32_t monster) = 0;
    [[nodiscard]] virtual std::uint32_t MonsterManager() = 0;
    virtual void IterateAndCallback(std::uint32_t manager,
                                    RemoveCallback callback,
                                    std::uint32_t context) = 0;
    [[nodiscard]] virtual std::int32_t Cleanup(
        std::uint32_t manager) = 0;
};

class PalConsoleMonsterCommands final {
public:
    // IDA 0x5bc8e0 / 0x5bc900.
    [[nodiscard]] static std::uint32_t RemoveCallback(
        std::uint32_t monster, IPalConsoleMonsterRuntime& runtime);
    [[nodiscard]] static std::int32_t KillAll(
        IPalConsoleMonsterRuntime& runtime);
};

}  // namespace pal4
