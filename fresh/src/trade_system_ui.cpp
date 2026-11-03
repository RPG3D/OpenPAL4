#include "pal4/trade_system_ui.h"

namespace pal4 {
namespace {

std::int32_t SendEvent(ITradeSystemUiRuntime& runtime,
                       const std::uint32_t event,
                       const std::uint32_t first,
                       const std::uint32_t second) {
    return runtime.ProcessEvent(runtime.UIFrameManager() + 260u,
                                event, first, second);
}

}  // namespace

std::uint8_t TradeSystemUi::OpenShop(
    const std::uint32_t shop_text, const std::uint8_t selector,
    const std::uint32_t goods_group, ITradeSystemUiRuntime& runtime) {
    runtime.CreateMusicText("p37.mp3");
    const auto player = runtime.BackgroundMusicPlayer();
    runtime.FadeInSpecialMusic(player, 3.0f, 0u);
    runtime.DestroyMusicText();
    state_ = 1u;

    const auto database = runtime.GameDatabaseInstance();
    const auto goods = runtime.FindShopGoods(database + 108u, shop_text);
    if (goods == 0u || goods_group >= 4u) return 0u;

    static_cast<void>(SendEvent(runtime, kEventTradeActive, 1u, 0u));
    static_cast<void>(SendEvent(runtime, kEventTradeSelector, selector, 0u));
    static_cast<void>(SendEvent(runtime, kEventTradeGoods,
                                goods_group, goods));
    runtime.ToggleWindows(runtime.UIFrameManager(), true);
    return 1u;
}

std::int32_t TradeSystemUi::CloseShop(ITradeSystemUiRuntime& runtime) {
    static_cast<void>(SendEvent(runtime, kEventTradeActive, 0u, 0u));
    runtime.ToggleWindows(runtime.UIFrameManager(), false);
    state_ = 0u;
    const auto scene_name = runtime.ReadWord(runtime.GameArenaManager() + 224u);
    const auto music_player = runtime.ReadWord(runtime.GameArenaManager() + 8u);
    return runtime.SetBackgroundMusic(runtime.BackgroundMusicPlayer(),
                                       music_player, scene_name);
}

std::uint8_t TradeSystemUi::SetTradeItemInfo(
    const std::uint32_t shop_id, const std::uint32_t task_script,
    ITradeSystemUiRuntime& runtime) {
    state_ = 0u;
    static_cast<void>(runtime.InitializeGameState(
        runtime.GameStateInstance(), 4u, 0u));
    static_cast<void>(SendEvent(runtime, kEventShopId, 1u, shop_id));
    static_cast<void>(SendEvent(runtime, kEventTradeTask, 1u, task_script));
    return static_cast<std::uint8_t>(SendEvent(
        runtime, kEventShowSelector, 1u, 0u));
}

std::uint8_t TradeSystemUi::ClearTradeItemInfo(
    ITradeSystemUiRuntime& runtime) {
    state_ = 0u;
    static_cast<void>(runtime.InitializeGameState(
        runtime.GameStateInstance(), 4u, 0u));
    return static_cast<std::uint8_t>(SendEvent(
        runtime, kEventShowSelector, 1u, 0u));
}

std::int32_t TradeSystemUi::SetTradeState(
    ITradeSystemUiRuntime& runtime) {
    const auto result = runtime.InitializeGameState(
        runtime.GameStateInstance(), 1u, 0u);
    state_ = 2u;
    return result;
}

}  // namespace pal4
