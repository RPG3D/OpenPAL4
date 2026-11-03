#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ITradeSystemUiRuntime {
public:
    virtual ~ITradeSystemUiRuntime() = default;
    virtual void CreateMusicText(std::string_view literal) = 0;
    [[nodiscard]] virtual std::uint32_t BackgroundMusicPlayer() = 0;
    virtual void FadeInSpecialMusic(std::uint32_t player, float seconds,
                                    std::uint32_t flags) = 0;
    virtual void DestroyMusicText() = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabaseInstance() = 0;
    [[nodiscard]] virtual std::uint32_t FindShopGoods(
        std::uint32_t table, std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    [[nodiscard]] virtual std::int32_t ProcessEvent(
        std::uint32_t tree, std::uint32_t event,
        std::uint32_t first, std::uint32_t second) = 0;
    virtual void ToggleWindows(std::uint32_t frame, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t GameArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t SetBackgroundMusic(
        std::uint32_t player, std::uint32_t music_player,
        std::uint32_t scene_name) = 0;
    [[nodiscard]] virtual std::uint32_t GameStateInstance() = 0;
    [[nodiscard]] virtual std::int32_t InitializeGameState(
        std::uint32_t game, std::uint32_t state, std::uint32_t argument) = 0;
};

class TradeSystemUi final {
public:
    static constexpr std::uint32_t kEventTradeActive = 0x8b0a44u;
    static constexpr std::uint32_t kEventTradeSelector = 0x8b0a4cu;
    static constexpr std::uint32_t kEventTradeGoods = 0x8b0a48u;
    static constexpr std::uint32_t kEventShopId = 0x8b0984u;
    static constexpr std::uint32_t kEventTradeTask = 0x8b0988u;
    static constexpr std::uint32_t kEventShowSelector = 0x8b0980u;

    explicit TradeSystemUi(std::uint32_t& state) : state_(state) {}

    // IDA 0x50fa00.
    [[nodiscard]] std::uint8_t OpenShop(
        std::uint32_t shop_text, std::uint8_t selector,
        std::uint32_t goods_group, ITradeSystemUiRuntime& runtime);
    // IDA 0x50fb90.
    [[nodiscard]] std::int32_t CloseShop(ITradeSystemUiRuntime& runtime);
    // IDA 0x50fc00.
    [[nodiscard]] std::uint8_t SetTradeItemInfo(
        std::uint32_t shop_id, std::uint32_t task_script,
        ITradeSystemUiRuntime& runtime);
    // IDA 0x50fca0.
    [[nodiscard]] std::uint8_t ClearTradeItemInfo(
        ITradeSystemUiRuntime& runtime);
    // IDA 0x50fcf0.
    [[nodiscard]] std::int32_t SetTradeState(
        ITradeSystemUiRuntime& runtime);

private:
    std::uint32_t& state_;
};

}  // namespace pal4
