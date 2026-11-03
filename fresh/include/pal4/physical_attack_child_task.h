#pragma once

#include "pal4/combat_script_task_operations.h"
#include "pal4/combat_process_message_task.h"
#include "pal4/combat_effect_slot.h"

#include <cstdint>

namespace pal4 {

class IPhysicalAttackChildRuntime : public ICombatScriptTaskRuntime,
                                    public ICombatEffectSlotRuntime {
public:
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) override = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void DestroyEffectSlot(std::uint32_t slot) = 0;
    virtual void DestroyCombatBase(std::uint32_t object) = 0;
    virtual void DestroyMessageBase(std::uint32_t object) = 0;
};

class IPhysicalAttackChildDestructionRuntime
    : public ICombatProcessMessageTaskRuntime {
public:
    virtual void DestroyEmbeddedBase(std::uint32_t embedded) = 0;
};

class PhysicalAttackChildTask final {
public:
    static constexpr std::uint32_t kSize = 292u;
    static constexpr std::uint32_t kVtable = 0x8459fcu;
    static constexpr std::uint32_t kConstructionVtable = 0x845a24u;

    // IDA 0x5657a0: factory behind PhysicalAttack child wrapper 0x845964.
    [[nodiscard]] static std::uint32_t Create(
        IPhysicalAttackChildRuntime& runtime);
    // IDA 0x565960: destroy embedded attack then message-task base.
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t object,
        IPhysicalAttackChildDestructionRuntime& runtime);
};

class ICombatMessageBaseDeletingRuntime {
public:
    virtual ~ICombatMessageBaseDeletingRuntime() = default;
    virtual void DestroyBase(std::uint32_t object) = 0;
    virtual void Free(std::uint32_t object) = 0;
};

class CombatMessageTaskBase final {
public:
    // IDA 0x565870: base deleting destructor used by 0x845a24.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, std::uint8_t flags,
        ICombatMessageBaseDeletingRuntime& runtime);
};

}  // namespace pal4
