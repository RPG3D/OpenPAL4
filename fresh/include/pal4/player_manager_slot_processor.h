#pragma once

#include <cstdint>

#include "pal4/player_manager_slot_data_copier.h"

namespace pal4 {

class IPlayerManagerSlotProcessRuntime {
public:
    virtual ~IPlayerManagerSlotProcessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void ClearCollectionFirstFive(std::uint32_t collections) = 0;
    virtual void ClearFixedSortedValues(std::uint32_t collections) = 0;
    virtual void ResetInventorySlots(std::uint32_t slots) = 0;
    virtual void CleanupScriptLists(std::uint32_t script) = 0;
    virtual void InitializeMember(std::uint32_t member) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t FindRecord(
        std::uint32_t table, std::uint32_t key) = 0;
    [[nodiscard]] virtual IPlayerManagerSlotCopyRuntime& CopyRuntime() = 0;
    virtual void ProcessCollection(std::uint32_t object) = 0;
    virtual void UpdateEvent(std::uint32_t script,
                             std::uint32_t mode,
                             std::uint32_t key) = 0;
    virtual void FinalizeScript(std::uint32_t script) = 0;
    [[nodiscard]] virtual std::uint32_t GetElementByIndex(
        std::uint32_t data, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t SetProperty(
        std::uint32_t data, std::uint32_t index,
        std::uint32_t value) = 0;
};

class PlayerManagerSlotProcessRuntimeStub :
    public IPlayerManagerSlotProcessRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void ClearCollectionFirstFive(std::uint32_t) override;
    void ClearFixedSortedValues(std::uint32_t) override;
    void ResetInventorySlots(std::uint32_t) override;
    void CleanupScriptLists(std::uint32_t) override;
    void InitializeMember(std::uint32_t) override;
    std::uint32_t GameDatabase() override;
    std::uint32_t FindRecord(std::uint32_t,
                             std::uint32_t) override;
    IPlayerManagerSlotCopyRuntime& CopyRuntime() override;
    void ProcessCollection(std::uint32_t) override;
    void UpdateEvent(std::uint32_t, std::uint32_t,
                     std::uint32_t) override;
    void FinalizeScript(std::uint32_t) override;
    std::uint32_t GetElementByIndex(std::uint32_t,
                                    std::uint32_t) override;
    std::uint32_t SetProperty(std::uint32_t, std::uint32_t,
                              std::uint32_t) override;

private:
    PlayerManagerSlotCopyRuntimeStub copy_runtime_;
};

class PlayerManagerSlotProcessor final {
public:
    // IDA 0x43ec80: refresh one player-manager slot from GameDB.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t object, IPlayerManagerSlotProcessRuntime& runtime);
};

}  // namespace pal4
