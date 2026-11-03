#pragma once
#include "pal4/trade_system_ui.h"
#include <functional>
#include <string>

namespace pal4 {
class NativeArenaLoadRuntime;
class NativeGameArenaManagerOwner;
class NativePalGameIVTextureStorage;
class NativeShopRecordCatalog;
class BackgroundMusicPlayer;
class IBackgroundMusicRuntime;
struct NativeTradeSystemUiServices {
    std::function<std::int32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t,bool)> edges;
    std::function<std::string_view(std::uint32_t)> shop_text;
};
// Borrows application owners. The catalog and music identities are adapter
// tokens, never legacy GameDB/VC6 object images to dereference.
class NativeTradeSystemUiRuntime final : public ITradeSystemUiRuntime {
public:
    NativeTradeSystemUiRuntime(NativeArenaLoadRuntime&,NativeGameArenaManagerOwner&,
        NativePalGameIVTextureStorage&,const NativeShopRecordCatalog&,
        pal4::BackgroundMusicPlayer&,IBackgroundMusicRuntime&,std::uint32_t manager,
        NativeTradeSystemUiServices);
    void CreateMusicText(std::string_view) override;
    std::uint32_t BackgroundMusicPlayer() override;
    void FadeInSpecialMusic(std::uint32_t,float,std::uint32_t) override;
    void DestroyMusicText() override;
    std::uint32_t GameDatabaseInstance() override;
    std::uint32_t FindShopGoods(std::uint32_t,std::uint32_t) override;
    std::uint32_t UIFrameManager() override;
    std::int32_t ProcessEvent(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override;
    void ToggleWindows(std::uint32_t,bool) override;
    std::uint32_t GameArenaManager() override;
    std::uint32_t ReadWord(std::uint32_t) override;
    std::int32_t SetBackgroundMusic(std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::uint32_t GameStateInstance() override;
    std::int32_t InitializeGameState(std::uint32_t,std::uint32_t,std::uint32_t) override;
private:
    NativeArenaLoadRuntime& transitions_;
    NativeGameArenaManagerOwner& arena_;
    NativePalGameIVTextureStorage& game_;
    const NativeShopRecordCatalog& catalog_;
    pal4::BackgroundMusicPlayer& music_;
    IBackgroundMusicRuntime& audio_;
    std::uint32_t manager_;
    NativeTradeSystemUiServices services_;
    std::string music_text_;
    bool music_text_active_=false;
};
}
