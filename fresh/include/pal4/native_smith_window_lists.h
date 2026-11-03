#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <functional>
namespace pal4 {
class NativeSmithWindowLayout;class NativeInventorySession;class NativePrescriptionRecordCatalog;class NativeGameDatabase;
class NativePlayerInventoryStateRuntime;
// Original reset/learned-recipe list and detail text composition. Lists own DLL
// cells and must close before the borrowed layout. No recipe copies are kept.
class NativeSmithWindowLists final {
public:
    NativeSmithWindowLists(NativeSmithWindowLayout&,NativeInventorySession&,const NativePrescriptionRecordCatalog&,const NativeGameDatabase&,NativePlayerInventoryStateRuntime&);
    ~NativeSmithWindowLists();
    NativeSmithWindowLists(const NativeSmithWindowLists&)=delete;
    NativeSmithWindowLists& operator=(const NativeSmithWindowLists&)=delete;
    void Reset();
    void Load(std::uint32_t mode);
    void ClearEquipmentSelection();
    void LoadPrescriptions(std::uint32_t mode);
    void LoadEquipment(std::uint32_t category);
    // Rebuild original category/instance selection for the hovered recipe.
    void SelectHoveredPrescription();
    void CollectMatchingMaterialInstances(std::uint32_t recipe);
    void RefreshCollectedEquipment();
    void CollectAvailableCategories(std::uint32_t recipe);
    bool FromDllCategory(std::uint32_t handler);
    bool FromDllHover(std::uint32_t handler);
    // Original49C4A0 selection-clear/hide/detail ordering. Callback is borrowed
    // for this call only and must hide the actual controller selection panel.
    void HandlePrescriptionClick(const std::function<void()>& hide_selection);
    void RefreshAvailability();
    void RefreshEquipmentAvailability();
    void RefreshPrescriptionAvailability();
    std::string EquipmentAttributeDescription(std::uint32_t sequence);
    std::size_t OwnedItemCount()const noexcept;
    void Close();
private:
    class Runtime;std::unique_ptr<Runtime> runtime_;
};
}
