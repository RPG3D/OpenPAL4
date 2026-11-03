#pragma once
#include "pal4/script_inventory_query.h"

namespace pal4 {
class NativeInventorySession;

// Read-only borrowed view of the existing inventory; never creates item records.
class NativeScriptInventoryQuery final : public IScriptInventoryQueryRuntime {
public:
    explicit NativeScriptInventoryQuery(NativeInventorySession& inventory) : inventory_(inventory) {}
    std::uint32_t Inventory() override;
    std::uint32_t FindElement(std::uint32_t, std::int32_t, std::uint32_t) override;
    std::uint32_t ReadWord(std::uint32_t) override;
private:
    NativeInventorySession& inventory_;
};
}
