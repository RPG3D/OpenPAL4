#pragma once
#include "pal4/native_common_dialog_layout.h"
#include <filesystem>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCeguiUiSession;class NativeInventorySession;struct PlayerManagerObjectState;
class NativeBaseObjectMemory;class NativePlayerScriptRefreshRuntime;class NativePlayerSkillRuntime;
class NativeGameDatabase;class NativeShopRecordCatalog;class NativePrescriptionRecordCatalog;
class NativeUiCollectionObjects;class NativeEffectSceneSession;class NativePkFileManagerRuntime;
class NativeEventHandlerArray;class NativeGameTimerQueue;class NativePalGameIVTextureStorage;
struct NativeTradeWindowServices {
    NativeCommonDialogLayout::SoundPlayer sound;
    std::function<void()> reset_trade;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
};
// Production composition of the original TradeWindow components. Borrows the
// application inventory/players/effects/activity/timers/dialog, not substitutes.
class NativeTradeWindow final {
public:
    NativeTradeWindow(NativeCeguiUiSession&,const std::filesystem::path&,NativeInventorySession&,
        PlayerManagerObjectState&,NativeBaseObjectMemory&,NativePlayerScriptRefreshRuntime&,NativePlayerSkillRuntime&,
        const NativeGameDatabase&,const NativeShopRecordCatalog&,const NativePrescriptionRecordCatalog&,
        NativeUiCollectionObjects&,NativeEffectSceneSession&,NativePkFileManagerRuntime&,NativeEventHandlerArray&,
        NativeGameTimerQueue&,std::uint32_t manager,NativePalGameIVTextureStorage&,
        NativeCommonDialogLayout&,NativeTradeWindowServices);
    ~NativeTradeWindow();
    NativeTradeWindow(const NativeTradeWindow&)=delete;
    NativeTradeWindow& operator=(const NativeTradeWindow&)=delete;
    std::uint32_t FrameAddress()const noexcept;
    std::uint32_t RootAddress()const noexcept;
    void SetVisible(bool,std::uint32_t parent=0u);
    bool Event(std::uint32_t key,std::uint32_t first,std::uint32_t second);
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t member,std::uint32_t slot);
    std::uint8_t Predicate(std::uint32_t member,std::uint32_t slot,std::uint32_t event);
    void TimerCallback(std::uint32_t owner,std::uint32_t method,std::int32_t row,std::uint8_t flag);
    void RethrowEventError();
    void Close();
private:
    class Impl;std::shared_ptr<Impl> impl_;
};
}
