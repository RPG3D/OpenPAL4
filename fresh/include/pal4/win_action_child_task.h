#pragma once

#include "pal4/combat_effect_slot.h"

#include <cstdint>

namespace pal4 {

class IWinActionSimpleCreationRuntime {
public:
    virtual ~IWinActionSimpleCreationRuntime()=default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t object) = 0;
    virtual void ConstructBase(std::uint32_t object) = 0;
    virtual void DestroyBase(std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value)=0;
};
class IWinActionChildCreationRuntime : public IWinActionSimpleCreationRuntime,public ICombatEffectSlotRuntime {
public:
    virtual void WriteWord(std::uint32_t address,std::uint32_t value) override=0;
    virtual void DestroyEffectSlot(std::uint32_t slot) = 0;
    virtual void DestroyMessageBase(std::uint32_t object) = 0;
};

class WinActionSecondChildTask final {
public:
    static constexpr std::uint32_t kSize = 64u;
    static constexpr std::uint32_t kVtable = 0x845a88u;

    // IDA 0x566650: factory behind WinAction wrapper 0x845a80.
    [[nodiscard]] static std::uint32_t Create(
        IWinActionSimpleCreationRuntime& runtime);
};

class WinActionThirdChildTask final {
public:
    static constexpr std::uint32_t kSize = 216u;
    static constexpr std::uint32_t kConstructionVtable = 0x845a24u;
    static constexpr std::uint32_t kVtable = 0x8456c4u;

    // IDA 0x5666c0: factory behind WinAction wrapper 0x845a84.
    [[nodiscard]] static std::uint32_t Create(
        IWinActionChildCreationRuntime& runtime);
};

}  // namespace pal4
