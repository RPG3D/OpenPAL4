#pragma once

#include "pal4/indexed_sorted_value_arrays.h"
#include "pal4/inventory_item_slots.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/player_manager_slot_collection_processor.h"

namespace pal4 {

class INativePlayerCollectionScriptRuntime {
public:
    virtual ~INativePlayerCollectionScriptRuntime() = default;
    virtual void HandleFade(std::uint32_t script, std::uint32_t value,
                            std::uint32_t marker) = 0;
    virtual void FinalizeScript(std::uint32_t script) = 0;
};

// Applies the rewritten collection stages to an owned x86 player slot image.
class NativePlayerSlotCollectionRuntime final :
    public IPlayerManagerSlotCollectionRuntime {
public:
    NativePlayerSlotCollectionRuntime(
        NativeBaseObjectMemory& memory, IInventoryItemSlotsRuntime& inventory,
        ISortedValueProcessRuntime& indexed_values,
        ISortedValueProcessRuntime& fixed_values,
        INativePlayerCollectionScriptRuntime& script)
        : memory_(memory), inventory_(inventory),
          indexed_values_(indexed_values), fixed_values_(fixed_values),
          script_(script) {}

    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t Free(std::uint32_t pointer) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void ProcessInventory(std::uint32_t slots,
                          PlayerManagerResultArray& result) override;
    void ProcessIndexedValues(std::uint32_t collection,
                              PlayerManagerResultArray& result) override;
    void ProcessFixedValues(std::uint32_t collection,
                            PlayerManagerResultArray& result) override;
    void HandleFade(std::uint32_t script, std::uint32_t value,
                    std::uint32_t marker) override;
    void FinalizeScript(std::uint32_t script) override;

private:
    NativeBaseObjectMemory& memory_;
    IInventoryItemSlotsRuntime& inventory_;
    ISortedValueProcessRuntime& indexed_values_;
    ISortedValueProcessRuntime& fixed_values_;
    INativePlayerCollectionScriptRuntime& script_;
};

}  // namespace pal4
