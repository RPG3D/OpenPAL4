#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
namespace pal4 {
class NativeTradeWindowLayout;class NativeInventorySession;
class NativePalGameIVTextureStorage;class NativeShopRecordCatalog;
class NativePrescriptionRecordCatalog;class NativeEventHandlerArray;
class NativeTradeWindowData final {
public:
    using Page=std::function<void(std::uint32_t group,std::uint32_t page,std::uint32_t flag)>;
    NativeTradeWindowData(NativeTradeWindowLayout&,NativeInventorySession&,
        NativePalGameIVTextureStorage&,const NativeShopRecordCatalog&,
        const NativePrescriptionRecordCatalog&,NativeEventHandlerArray&,
        std::uint32_t manager,Page);
    ~NativeTradeWindowData();
    NativeTradeWindowData(const NativeTradeWindowData&)=delete;
    NativeTradeWindowData& operator=(const NativeTradeWindowData&)=delete;
    void SetActive(std::uint32_t selector);
    void Refresh(std::uint32_t group,std::uint32_t shop_record);
    std::span<const std::uint32_t> Records(std::uint32_t group)const;
    bool HasItems();
    bool CanChangePage();
    // Before layout/effects/inventory retirement. Clears the same frame arrays.
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
