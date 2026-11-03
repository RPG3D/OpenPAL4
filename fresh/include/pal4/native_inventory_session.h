#pragma once

#include "pal4/inventory_item_slots.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_equipment_record_catalog.h"
#include "pal4/native_property_record_catalog.h"

#include <cstddef>
#include <functional>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace pal4 {

// Owns the original 0x138-byte inventory image used by the player slots.
class NativeInventorySession final : public IInventoryItemSlotsRuntime {
public:
    NativeInventorySession() = default;
    NativeInventorySession(const NativeInventorySession&) = delete;
    NativeInventorySession& operator=(const NativeInventorySession&) = delete;
    ~NativeInventorySession() override;

    [[nodiscard]] std::uint32_t Start();
    void Close();
    // IDA4C60F0: clear items/equipment/categories without replacing the owner.
    void Reset();
    // IDA4C6010: shared signed-add/clamp semantics, including 32-bit wrap.
    [[nodiscard]] std::int32_t UpdateCounter(std::int32_t delta);
    void BindDatabase(const NativeGameDatabase& database);
    void BindItemObserver(std::function<void(std::uint32_t)> observer) { item_observer_ = std::move(observer); }
    void BindEquipmentObserver(std::function<void(std::uint32_t)> observer) { equipment_observer_=std::move(observer); }
    bool RewardObserversBound() const noexcept { return bool(item_observer_) && bool(equipment_observer_); }
    std::size_t PropertyRecordCount() const noexcept { return properties_?properties_->Size():0u; }
    std::uint32_t PropertyRecordAt(std::size_t index) const;
    [[nodiscard]] std::uint32_t PropertyAddress(std::uint32_t id) const noexcept;
    [[nodiscard]] std::uint32_t AddItem(std::uint32_t id, std::int32_t count);
    [[nodiscard]] std::uint32_t RemoveItem(std::uint32_t id, std::int32_t count);
    [[nodiscard]] std::uint32_t ItemCount(std::uint32_t id);
    // IDA FindElementInGameDB category5: borrowed owned-item entry, zero if absent.
    [[nodiscard]] std::uint32_t FindItemById(std::uint32_t id);
    void SetRecentItem(std::uint32_t category,std::uint32_t sequence);
    [[nodiscard]] bool HasEquipment(std::uint32_t id) const noexcept;
    // Borrowed original definition, distinct from an owned equipment instance.
    [[nodiscard]] std::uint32_t EquipmentDefinitionAddress(std::uint32_t id) const noexcept;
    // Read-only equipment instance views; addresses expire on mutation/reset.
    [[nodiscard]] std::uint32_t EquipmentCount(std::uint32_t type);
    [[nodiscard]] std::uint32_t EquipmentAt(std::uint32_t type, std::uint32_t index);
    [[nodiscard]] std::vector<std::uint32_t> RecentEquipmentSequences();
    // IDA4C4740: removes this instance sequence, retaining other copies.
    [[nodiscard]] bool RemoveEquipment(std::uint32_t sequence);
    [[nodiscard]] std::uint32_t ProcessEquipment(std::uint32_t collection,
                                                  std::uint32_t id);
    [[nodiscard]] std::uint32_t Address() const noexcept { return address_; }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }

    std::uint32_t ReadDword(std::uint32_t address) override;
    std::uint32_t GetGameDatabase() override;
    std::uint32_t ProcessGameDatabaseArray(std::uint32_t array,
                                           std::uint32_t value) override;
    std::uint32_t GetInventory() override;
    std::uint32_t FindInventoryEntry(std::uint32_t slots, std::uint32_t id,
                                     std::uint32_t category) override;
    std::uint32_t ProcessEntry(std::uint32_t first, std::uint32_t second,
                               std::uint32_t entry) override;
    void InsertEntry(std::uint32_t destination, std::uint32_t entry) override;

private:
    friend class NativeInventoryItemRuntime;
    std::function<void(std::uint32_t)> item_observer_;
    std::function<void(std::uint32_t)> equipment_observer_;
    friend class NativeInventoryEquipmentReader;
    friend class NativeInventoryEquipmentMutation;
    friend class NativeInventoryPrescriptions;
    friend class NativeInventoryLoadRuntime;
    void ResetEquipment();
    void ResetCategories();
    [[nodiscard]] std::uint32_t ResetItems();
    NativeBaseObjectMemory memory_;
    std::unique_ptr<NativeEquipmentRecordCatalog> equipment_;
    std::unique_ptr<NativePropertyRecordCatalog> properties_;
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> by_id_;
    std::uint32_t address_ = 0u;
};

}  // namespace pal4
