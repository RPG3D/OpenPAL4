#pragma once

#include "pal4/combat_system_text_members.h"

#include <cstdint>

namespace pal4 {

enum class CombatSystemMember {
    system_manager,
    event_router,
    script_handlers,
    extended_handlers,
    game_event_handler,
    auxiliary,
    actor_manager,
};

class ICombatSystemObjectRuntime : public ICombatSystemTextRuntime {
public:
    virtual ~ICombatSystemObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void ConstructWorld(std::uint32_t object) = 0;
    virtual void ConstructTranslator(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructMember(
        CombatSystemMember member, std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructDataStructure() = 0;
    virtual void InitializeTail(std::uint32_t tail) = 0;
    virtual void DestroyMember(CombatSystemMember member,
                               std::uint32_t object) = 0;
    virtual void ClearCombatEvents(std::uint32_t collection) = 0;
    virtual void DestroyTimerQueue(std::uint32_t object) = 0;
    virtual void DestroyVirtual(std::uint32_t object,
                                std::uint32_t deleting_flag) = 0;
    virtual void DestructTranslator(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t DestructWorld(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint8_t AreAllObjectsReady(
        std::uint32_t system_manager) = 0;
};

class CombatSystemObject final {
public:
    // The original constructor reads an indeterminate stack byte twice.
    // Make it explicit so both writes have a defined, testable value.
    // IDA 0x574fd0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t data_flag_seed,
        ICombatSystemObjectRuntime& runtime);
    // IDA 0x5751e0.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, ICombatSystemObjectRuntime& runtime);
    // IDA 0x575460, 0x575470, 0x575490 and 0x5754b0.
    [[nodiscard]] static std::uint8_t IsInactive(
        std::uint32_t object, ICombatSystemObjectRuntime& runtime);
    [[nodiscard]] static bool IsActive(
        std::uint32_t object, ICombatSystemObjectRuntime& runtime);
    [[nodiscard]] static bool IsEnding(
        std::uint32_t object, ICombatSystemObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetCamera(
        std::uint32_t object, ICombatSystemObjectRuntime& runtime);
    // IDA 0x5767f0.
    [[nodiscard]] static std::uint8_t AreAllObjectsReady(
        std::uint32_t object, ICombatSystemObjectRuntime& runtime);
    // IDA 0x576bd0 and 0x576be0.
    [[nodiscard]] static std::uint32_t SetProperty110(
        std::uint32_t object, std::uint32_t value,
        ICombatSystemObjectRuntime& runtime);
    [[nodiscard]] static std::uint8_t GetProperty519(
        std::uint32_t object, ICombatSystemObjectRuntime& runtime);
};

}  // namespace pal4
