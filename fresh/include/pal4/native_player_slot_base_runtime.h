#pragma once

#include "pal4/base_object_activity.h"
#include "pal4/player_manager_object.h"
#include "pal4/player_manager_slot.h"

namespace pal4 {

// Runs the inherited BaseObject lifecycle over the caller's player-slot image.
// Nested resources and scene operations remain services of the game host.
class NativePlayerSlotBaseRuntime final : public IPlayerManagerSlotRuntime {
public:
    NativePlayerSlotBaseRuntime(PlayerManagerObjectState& manager,
                                IPlayerManagerSlotRuntime& host,
                                IBaseObjectRuntime& lifecycle,
                                IBaseObjectActivityRuntime& activity)
        : manager_(manager), host_(host), lifecycle_(lifecycle),
          activity_(activity) {}

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
    [[nodiscard]] BaseObjectState& Base(std::uint32_t object);

    PlayerManagerObjectState& manager_;
    IPlayerManagerSlotRuntime& host_;
    IBaseObjectRuntime& lifecycle_;
    IBaseObjectActivityRuntime& activity_;
};

}  // namespace pal4
