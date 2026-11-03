#pragma once
#include <cstdint>
#include <functional>
#include <string_view>
namespace pal4 {
class NativePrescriptionRecordCatalog;
class NativeInventorySession;
// Original5920E0 primitive; original Smith selection/material gates precede it.
class NativeCraftingEnchantment final {
public:
    using SoundPlayer=std::function<void(std::string_view,std::uint32_t)>;
    NativeCraftingEnchantment(const NativePrescriptionRecordCatalog&,NativeInventorySession&,SoundPlayer);
    std::int32_t Apply(std::uint32_t recipe,std::uint32_t target_sequence,bool opened_from_system);
private:
    const NativePrescriptionRecordCatalog& recipes_;
    NativeInventorySession& inventory_;
    SoundPlayer sound_;
};
}
