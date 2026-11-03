#pragma once

#include "pal4/game_data_structure_state.h"
#include "pal4/player_manager_slot_processor.h"
#include "pal4/script_skill_collection.h"

namespace pal4 {

class NativeRoleRecordCatalog;

class INativePlayerSkillCleanupCallbacks {
public:
    virtual ~INativePlayerSkillCleanupCallbacks() = default;
    [[nodiscard]] virtual std::uint32_t ResolveSkillRecord(
        std::uint32_t skill) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeVirtual(
        std::uint32_t receiver, std::uint32_t vtable_offset,
        std::span<const std::uint32_t> arguments) = 0;
};

// Applies the rewritten in-place slot resets to native x86 memory. A supplied
// Role catalog also resolves the original post-load Role lookup. The remaining
// script and event operations have explicit owners.
class NativePlayerSlotProcessRuntime final :
    public IPlayerManagerSlotProcessRuntime,
    private IGameDataStructureStateRuntime,
    private IScriptSkillListCleanupRuntime {
public:
    explicit NativePlayerSlotProcessRuntime(
        IPlayerManagerSlotProcessRuntime& remaining,
        INativePlayerSkillCleanupCallbacks& skill_callbacks,
        const NativeRoleRecordCatalog* roles = nullptr)
        : remaining_(remaining), skill_callbacks_(skill_callbacks),
          roles_(roles) {}

    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    void ClearCollectionFirstFive(std::uint32_t collections) override;
    void ClearFixedSortedValues(std::uint32_t collections) override;
    void ResetInventorySlots(std::uint32_t slots) override;
    void CleanupScriptLists(std::uint32_t script) override;
    void ResetScriptProperties(std::uint32_t script);
    void InitializeMember(std::uint32_t member) override;
    std::uint32_t GameDatabase() override;
    std::uint32_t FindRecord(std::uint32_t table,
                             std::uint32_t key) override;
    IPlayerManagerSlotCopyRuntime& CopyRuntime() override;
    void ProcessCollection(std::uint32_t object) override;
    void UpdateEvent(std::uint32_t script, std::uint32_t mode,
                     std::uint32_t key) override;
    void FinalizeScript(std::uint32_t script) override;
    std::uint32_t GetElementByIndex(std::uint32_t data,
                                    std::uint32_t index) override;
    std::uint32_t SetProperty(std::uint32_t data,
                              std::uint32_t index,
                              std::uint32_t value) override;

private:
    void InitializeOuterBase(std::uint32_t object) override;
    void InitializeInnerBase(std::uint32_t object) override;
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t ResolveSkillRecord(std::uint32_t skill) override;
    std::uint32_t InvokeVirtual(
        std::uint32_t receiver, std::uint32_t vtable_offset,
        std::span<const std::uint32_t> arguments) override;

    IPlayerManagerSlotProcessRuntime& remaining_;
    INativePlayerSkillCleanupCallbacks& skill_callbacks_;
    const NativeRoleRecordCatalog* roles_;
    std::uint32_t database_{};
};

}  // namespace pal4
