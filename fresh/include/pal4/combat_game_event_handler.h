#pragma once

#include "pal4/combat_script_event_handlers.h"

#include <cstdint>

namespace pal4 {

class ICombatGameStatsRuntime {
public:
    virtual ~ICombatGameStatsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetGameDatabase() = 0;
    // Native catalogs return a game-specific32-word record directly, without
    // constructing the legacy database's index/vector implementation.
    [[nodiscard]] virtual std::uint32_t FindStatRecord(std::int32_t key);
    [[nodiscard]] virtual std::uint32_t GetCombatSystem() = 0;
    [[nodiscard]] virtual bool IsCombatSystemInactive(
        std::uint32_t combat_system) = 0;
};

class ICombatGameEventHandlerRuntime : public ICombatGameStatsRuntime,
                                       public ICombatScriptEventHandlerRuntime {
public:
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) override = 0;
};

class ICombatGameStatUpdateRuntime {
public:
    virtual ~ICombatGameStatUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetScriptEventRouter() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessScoreChanged(
        std::uint32_t router, std::uint32_t descriptor,
        std::uint32_t delta, std::uint32_t mode) = 0;
};

class CombatGameStats final {
public:
    // IDA 0x5743e0. Keeps source-dependent fields when either record is absent.
    static void Initialize(std::uint32_t object,
                           ICombatGameStatsRuntime& runtime);
    // IDA 0x574560.
    [[nodiscard]] static std::uint32_t CalculateTotal(
        std::uint32_t object, ICombatGameStatsRuntime& runtime);
    // IDA 0x574590.
    [[nodiscard]] static std::uint32_t GetBattleRank(
        std::uint32_t object, std::int32_t value,
        ICombatGameStatsRuntime& runtime);
    // IDA 0x574bc0.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object, std::uint32_t stat,
        std::uint32_t increment,
        ICombatGameStatUpdateRuntime& runtime);
};

class CombatGameEventHandler final {
public:
    // IDA 0x5743b0. Initializes the 620-byte event/statistics object.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ICombatGameEventHandlerRuntime& runtime);
};

}  // namespace pal4
