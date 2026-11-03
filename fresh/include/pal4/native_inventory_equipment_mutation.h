#pragma once
#include <cstdint>
#include <string_view>
namespace pal4 {
class NativeInventorySession;
// Mutates an existing instance by sequence. Text remains owned by inventory;
// definition/type/sequence and collection keys cannot be changed through here.
class NativeInventoryEquipmentMutation final {
public:
    explicit NativeInventoryEquipmentMutation(NativeInventorySession& inventory):inventory_(inventory){}
    void WriteScalar(std::uint32_t sequence,std::uint32_t offset,std::uint32_t bits);
    void AssignText(std::uint32_t sequence,std::uint32_t offset,std::string_view value);
private:
    std::uint32_t Entry(std::uint32_t sequence);
    NativeInventorySession& inventory_;
};
}
