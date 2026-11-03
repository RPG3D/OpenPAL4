#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class GameDataEffectTableKind {
    role,
    upgrade,
    combat_prize,
};

class IGameDataEffectTableBindingRuntime {
public:
    virtual ~IGameDataEffectTableBindingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t LogBind(
        std::string_view prefix, std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeStructures(
        GameDataEffectTableKind kind, std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
};

class GameDataEffectTableBinding final {
public:
    // IDA 0x4ed200, 0x4ed430, 0x4ed660.
    [[nodiscard]] static std::uint32_t RefreshRole(
        std::uint32_t table, IGameDataEffectTableBindingRuntime& runtime);
    [[nodiscard]] static std::uint32_t RefreshUpgrade(
        std::uint32_t table, IGameDataEffectTableBindingRuntime& runtime);
    [[nodiscard]] static std::uint32_t RefreshCombatPrize(
        std::uint32_t table, IGameDataEffectTableBindingRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Refresh(
        std::uint32_t table, GameDataEffectTableKind kind,
        IGameDataEffectTableBindingRuntime& runtime);
};

}  // namespace pal4
