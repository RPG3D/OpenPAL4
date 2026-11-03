#pragma once

#include "pal4/combat_effect_slot.h"
#include "pal4/throw_property_task.h"

#include <cstdint>

namespace pal4 {

class ICombatPropertyActionCreationRuntime : public ICombatEffectSlotRuntime,
                                             public IThrowPropertyConstructionRuntime {
public:
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) override = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestroyGenericBase(std::uint32_t object) = 0;
    virtual void DestroyCombatBase(std::uint32_t object) = 0;
    virtual void DestroyMessageBase(std::uint32_t object) = 0;
    virtual void DestroyEffectSlot(std::uint32_t slot) = 0;
    virtual void SetTextErrorFlag(std::uint32_t header,
                                  std::uint8_t flag) = 0;
    virtual void ResetNameNode(std::uint32_t node) = 0;
    virtual void CleanupCastEmbedded(std::uint32_t embedded) = 0;
    virtual void DestroyThrowProperty(std::uint32_t embedded) = 0;
};

}  // namespace pal4
