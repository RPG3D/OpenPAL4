#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;class NativeGameDatabase;class NativeShopRecordCatalog;
class NativeTradeSystemOwner;class NativePalGameIVTextureStorage;class NativeEventHandlerArray;
class IShopUiActionsRuntime;
class IGameArenaLoadRuntime;class ITradeSystemUiRuntime;class NativeShopSelectorLayout;
struct NativeShopSelectorServices {
    std::function<void(std::uint32_t)> tutorial;
    // Borrow this selector's actions/task text for NativeShopTaskScript.
    // The application supplies its current arena/source and DispatchUiScript.
    std::function<bool(std::uint32_t frame,std::uint32_t event,
        IShopUiActionsRuntime&,std::string_view task_name)> task;
};
// Lazily owns original selector layout, availability and input. Borrows the
// game's existing trade UI and XML/state services. Retire outside DLL callbacks.
class NativeShopSelectorWindow final {
public:
    NativeShopSelectorWindow(NativeCeguiUiSession&,const std::filesystem::path&,
        const NativeGameDatabase&,const NativeShopRecordCatalog&,NativeTradeSystemOwner&,
        NativePalGameIVTextureStorage&,NativeEventHandlerArray&,std::uint32_t manager,
        ITradeSystemUiRuntime&,IGameArenaLoadRuntime&,NativeShopSelectorServices);
    ~NativeShopSelectorWindow();
    NativeShopSelectorWindow(const NativeShopSelectorWindow&)=delete;
    NativeShopSelectorWindow& operator=(const NativeShopSelectorWindow&)=delete;
    bool Event(std::uint32_t key,std::uint32_t first,std::uint32_t second);
    void SetVisible(bool,std::uint32_t parent=0u);
    bool Visible()const noexcept;
    NativeShopSelectorLayout* Layout()const noexcept;
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t,std::uint32_t);
    std::uint8_t Predicate(std::uint32_t,std::uint32_t,std::uint32_t);
    void RethrowError();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
