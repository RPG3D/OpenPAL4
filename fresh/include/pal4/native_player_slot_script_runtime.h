#pragma once

#include "pal4/player_manager_slot.h"
#include "pal4/script_skill_collection.h"

namespace pal4 {

// Initializes the embedded script collection while forwarding other slot work.
class NativePlayerSlotScriptRuntime final : public IPlayerManagerSlotRuntime,
                                            private IScriptSkillCollectionInitRuntime {
public:
    explicit NativePlayerSlotScriptRuntime(IPlayerManagerSlotRuntime& host)
        : host_(host) {}

    void BaseConstruct(std::uint32_t object) override;
    void BaseReset(std::uint32_t object) override;
    std::uint32_t CleanupWeaponEffects(std::uint32_t object) override;
    std::uint32_t BaseDestroy(std::uint32_t object) override;
    void InitializeSixWords(std::uint32_t address) override;
    void ConstructBlockArray(std::uint32_t address, std::uint32_t count,
                             std::uint32_t element_bytes) override;
    std::uint32_t DestroyBlockArray(std::uint32_t address,
                                    std::uint32_t count,
                                    std::uint32_t element_bytes) override;
    void InitializeScript(std::uint32_t address) override;
    void CleanupScriptOnUnwind(std::uint32_t address) override;
    void ResetScriptVtable(std::uint32_t address) override;
    void InitializeResourceArrays(std::uint32_t address) override;
    void CleanupResourceArraysOnUnwind(std::uint32_t address) override;
    void DestroyResourceArray(std::uint32_t address, std::uint32_t count,
                              std::uint32_t element_bytes) override;
    void InitializeTail(std::uint32_t address) override;
    void CleanupTail(std::uint32_t address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void Free(std::uint32_t address) override;

private:
    void InitializeOuterBase(std::uint32_t address) override;
    void InitializeInnerBase(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;

    IPlayerManagerSlotRuntime& host_;
};

}  // namespace pal4
