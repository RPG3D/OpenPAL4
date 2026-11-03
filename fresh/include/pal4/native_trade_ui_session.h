#pragma once
#include "pal4/native_shop_selector_window.h"
#include "pal4/native_trade_window.h"
#include <functional>
#include <memory>
namespace pal4 {
struct NativeTradeUiSessionServices {
    NativeShopSelectorServices selector;
    NativeCommonDialogLayout::SoundPlayer sound;
    std::function<std::unique_ptr<NativeTradeWindow>(NativeTradeWindowServices)> window;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
};
// Persistent global shop route; page resources are lazy and expire before the
// borrowed UI world. All TradeSystem operations use its actual native runtime.
class NativeTradeUiSession final {
public:
    NativeTradeUiSession(NativeCeguiUiSession&,const std::filesystem::path&,
        const NativeGameDatabase&,const NativeShopRecordCatalog&,NativeTradeSystemOwner&,
        NativePalGameIVTextureStorage&,NativeEventHandlerArray&,std::uint32_t manager,
        ITradeSystemUiRuntime&,IGameArenaLoadRuntime&,NativeTradeUiSessionServices);
    ~NativeTradeUiSession();
    NativeTradeUiSession(const NativeTradeUiSession&)=delete;
    NativeTradeUiSession& operator=(const NativeTradeUiSession&)=delete;
    bool Event(std::uint32_t key,std::uint32_t first,std::uint32_t second);
    std::uint8_t SetTradeItemInfo(std::string_view shop,std::string_view task);
    std::uint8_t ClearTradeItemInfo();
    std::int32_t CloseShop();
    std::string_view ShopText(std::uint32_t descriptor)const;
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t,std::uint32_t);
    std::uint8_t Predicate(std::uint32_t,std::uint32_t,std::uint32_t);
    void TimerCallback(std::uint32_t,std::uint32_t,std::int32_t,std::uint8_t);
    NativeShopSelectorWindow& Selector();
    NativeTradeWindow* Window()const noexcept;
    void ReleaseWorldPages();
    void RethrowError();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
