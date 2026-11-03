#pragma once

#include "pal4/collection_slot_group.h"
#include "pal4/animation_event_queue.h"
#include "pal4/game_data_role_record.h"
#include "pal4/game_object_text_arrays.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/player_manager_slot.h"

namespace pal4 {

// Binds the already rewritten six-word and 16-byte array members of a slot.
// Other player, script and resource services stay with the owning game host.
class NativePlayerSlotArrayRuntime final : public IPlayerManagerSlotRuntime,
                                           private ICollectionSlotGroupRuntime,
                                           private IGameDataRoleRecordRuntime,
                                           private IGameObjectTextCleanupRuntime {
public:
    NativePlayerSlotArrayRuntime(IPlayerManagerSlotRuntime& host,
                                 NativePlayerSlotCopyRuntime& text,
                                 IAnimationEventQueueRuntime& events)
        : host_(host), text_(text), events_(events) {}

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
    void DestroyElement(std::uint32_t element);
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void DestroyEntry(std::uint32_t entry) override;
    void FreeMemory(std::uint32_t address) override;
    void InitializeText(std::uint32_t address) override;
    void DestroyTextRecord(std::uint32_t address) override;
    void ReportInvalidSlot(std::int32_t index,
                           std::uint32_t source_line) override;

    IPlayerManagerSlotRuntime& host_;
    NativePlayerSlotCopyRuntime& text_;
    IAnimationEventQueueRuntime& events_;
};

}  // namespace pal4
