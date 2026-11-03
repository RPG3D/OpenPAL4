#pragma once

#include "pal4/combat_event_state_updater.h"
#include "pal4/game_data_role_database.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/native_upgrade_record_catalog.h"

#include <array>
#include <cstdint>
#include <vector>

namespace pal4 {

// Binds the original UpgradeData records and player slot image to the
// rewritten IDA 0x581690 / 0x57DB70 event path.
class NativePlayerEventRuntime final : public ICombatEventStateUpdateRuntime,
                                       public ICombatEventDataRuntime {
public:
    NativePlayerEventRuntime(const NativeUpgradeRecordCatalog& records,
                             NativeBaseObjectMemory& memory,
                             NativePlayerScriptRefreshRuntime& refresh);
    NativePlayerEventRuntime(const NativePlayerEventRuntime&) = delete;
    NativePlayerEventRuntime& operator=(const NativePlayerEventRuntime&) = delete;
    NativePlayerEventRuntime(NativePlayerEventRuntime&&) = delete;
    NativePlayerEventRuntime& operator=(NativePlayerEventRuntime&&) = delete;

    [[nodiscard]] std::int32_t Update(std::uint32_t script,
                                       std::int32_t level,
                                       std::uint32_t player_index);
    //4E9BF0 selector over this same original UpgradeData owner.
    std::int32_t LevelForExperience(std::uint32_t role_id,std::int32_t experience) const;

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::int32_t SetSlot(std::uint32_t actor,
                                       std::uint32_t slot,
                                       std::int32_t value) override;
    [[nodiscard]] std::int32_t QuerySlot(std::uint32_t actor,
                                         std::uint32_t slot) override;
    void RestoreSlot(std::uint32_t actor, std::uint32_t slot,
                     std::int32_t value) override;
    void Notify(std::uint32_t actor) override;
    [[nodiscard]] std::int32_t BaseStat(std::uint32_t actor,
                                        std::uint32_t slot) override;

    [[nodiscard]] std::uint32_t GameDatabase() override;
    [[nodiscard]] std::uint32_t InitializeMatches(
        std::uint8_t key_low_byte) override;
    [[nodiscard]] std::uint32_t UpgradeRecordCount(
        std::uint32_t count_holder) override;
    void InsertMatch(std::uint32_t collection,
                     std::uint32_t entry) override;
    [[nodiscard]] std::uint32_t MatchCount(
        std::uint32_t collection) override;
    [[nodiscard]] std::uint32_t FirstMatch(
        std::uint32_t collection) override;
    [[nodiscard]] std::int32_t LowerBound(std::uint32_t table,
                                          std::uint32_t first_key,
                                          std::uint32_t second_key) override;
    [[nodiscard]] std::int32_t UpperBound(std::uint32_t table,
                                          std::uint32_t first_key,
                                          std::uint32_t second_key) override;
    void SetValidatedProperty(std::uint32_t actor,
                              std::uint32_t property,
                              std::int32_t value) override;
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t count) override;
    void CleanupMatchRange(std::uint32_t collection) override;
    [[nodiscard]] int FreeMatches(std::uint32_t collection) override;

private:
    void CheckCollection(std::uint32_t collection) const;
    void CheckSelectorTable(std::uint32_t table) const;

    const NativeUpgradeRecordCatalog& records_;
    NativeBaseObjectMemory& memory_;
    NativePlayerScriptRefreshRuntime& refresh_;
    std::array<std::uint32_t, 12u> database_{};
    std::array<std::uint32_t, 35u> table_{};
    std::vector<GameDataUpgradeRecordView> views_;
    std::vector<std::uint32_t> matches_;
};

}  // namespace pal4
