#pragma once
#include <cstdint>
#include <vector>

namespace pal4 {
class NativeInventorySession;
class NativePrescriptionRecordCatalog;
struct NativeCraftingDeficit {
    std::uint32_t item{}, count{};
    bool operator==(const NativeCraftingDeficit&) const = default;
};
// Read-only bridge to the existing IDA590B70..590E70 algorithms.
// Catalog and inventory must outlive this view; queries own no inventory data.
class NativeCraftingRequirements final {
public:
    NativeCraftingRequirements(const NativePrescriptionRecordCatalog&, NativeInventorySession&);
    bool AreMet(std::uint32_t recipe) const;
    std::vector<NativeCraftingDeficit> Deficits(std::uint32_t recipe) const;
    bool HasEquipmentMaterial(std::uint32_t recipe) const;
    std::int32_t FirstEquipmentValue(std::uint32_t recipe) const;
    std::uint32_t Product(std::uint32_t recipe) const;
    std::int32_t EquipmentPotential(std::uint32_t sequence) const;
    bool MeetsRecipeProperty(std::uint32_t recipe,std::uint32_t sequence) const;
    bool ProductAtLimit(std::uint32_t definition) const;
    bool BelowRecipePotential(std::uint32_t recipe,std::uint32_t sequence) const;
    bool HasEnchantment(std::uint32_t sequence) const;
    bool MatchesRecipeEnchantment(std::uint32_t sequence,std::uint32_t recipe) const;
private:
    const NativePrescriptionRecordCatalog& recipes_;
    NativeInventorySession& inventory_;
};
}
