#pragma once
#include <cstdint>
namespace pal4 {
class NativeInventorySession;
class NativePrescriptionRecordCatalog;
// Live Inventory+104 three-category learned recipe collection. No second list.
class NativeInventoryPrescriptions final {
public:
    NativeInventoryPrescriptions(NativeInventorySession& inventory,const NativePrescriptionRecordCatalog& recipes):inventory_(inventory),recipes_(recipes){}
    std::uint32_t Add(std::uint32_t id);
    bool Contains(std::int32_t id);
    std::int32_t Count(std::int32_t mode);
    std::uint32_t At(std::int32_t mode,std::uint32_t index);
private:
    class Runtime;
    NativeInventorySession& inventory_;
    const NativePrescriptionRecordCatalog& recipes_;
};
}
