#pragma once
#include <windows.h>
#include <array>
#include <memory>
#include <string_view>
#include <cstdint>
namespace pal4 {
// Owns the original532-byte game-derived OLDefaultItem. DLL lists borrow it;
// reset/detach the list before destroying the wrapper.
class NativeEquipmentListItem final {
public:
    NativeEquipmentListItem(HMODULE,HMODULE,std::uint32_t list,std::string_view utf8,
        std::uint32_t sequence,bool equipped,bool disabled);
    ~NativeEquipmentListItem();
    NativeEquipmentListItem(const NativeEquipmentListItem&)=delete;
    NativeEquipmentListItem& operator=(const NativeEquipmentListItem&)=delete;
    std::uint32_t Address()const noexcept;
private:
    class Runtime;
    static void __fastcall Draw(void*,void*,void*,const std::array<float,4>&,float,float,const std::array<float,4>*,std::uint32_t);
    static void* __fastcall Delete(void*,void*,unsigned);
    void Release();
    HMODULE module_,look_;
    std::unique_ptr<std::byte[]> image_;
    std::array<std::uint32_t,8> vtable_{};
    std::uint32_t base_table_{};
    std::unique_ptr<Runtime> runtime_;
    bool constructed_{};
};
}
