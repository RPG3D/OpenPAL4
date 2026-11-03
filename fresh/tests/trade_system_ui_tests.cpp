#include "pal4/trade_system_ui.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string_view operation;
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    std::uint32_t third = 0u;
    std::uint32_t fourth = 0u;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::ITradeSystemUiRuntime {
    std::vector<Call> calls;
    std::uint32_t goods = 0xa000u;
    std::int32_t event_result = 0x1234;
    std::int32_t state_result = -17;

    void CreateMusicText(const std::string_view literal) override {
        Check(literal == "p37.mp3", "trade music path matches IDA");
        calls.push_back({"music-text"});
    }
    std::uint32_t BackgroundMusicPlayer() override {
        calls.push_back({"music-player"});
        return 0x7000u;
    }
    void FadeInSpecialMusic(const std::uint32_t player,
                            const float seconds,
                            const std::uint32_t flags) override {
        Check(seconds == 3.0f, "trade music fades in over three seconds");
        calls.push_back({"fade", player, flags});
    }
    void DestroyMusicText() override {
        calls.push_back({"music-text-destroy"});
    }
    std::uint32_t GameDatabaseInstance() override {
        calls.push_back({"database"});
        return 0x9000u;
    }
    std::uint32_t FindShopGoods(const std::uint32_t table,
                                const std::uint32_t text) override {
        calls.push_back({"find-goods", table, text});
        return goods;
    }
    std::uint32_t UIFrameManager() override {
        calls.push_back({"ui-frame"});
        return 0x8000u;
    }
    std::int32_t ProcessEvent(const std::uint32_t tree,
                              const std::uint32_t event,
                              const std::uint32_t first,
                              const std::uint32_t second) override {
        calls.push_back({"event", tree, event, first, second});
        return event_result;
    }
    void ToggleWindows(const std::uint32_t frame,
                       const bool visible) override {
        calls.push_back({"toggle", frame, visible ? 1u : 0u});
    }
    std::uint32_t GameArenaManager() override {
        calls.push_back({"arena"});
        return 0xb000u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back({"read", address});
        if (address == 0xb0e0u) return 0xd000u;
        Check(address == 0xb008u, "arena music player field offset");
        return 0xc000u;
    }
    std::int32_t SetBackgroundMusic(
        const std::uint32_t player, const std::uint32_t music_player,
        const std::uint32_t scene_name) override {
        calls.push_back({"set-music", player, music_player, scene_name});
        return -11;
    }
    std::uint32_t GameStateInstance() override {
        calls.push_back({"game-state"});
        return 0xe000u;
    }
    std::int32_t InitializeGameState(
        const std::uint32_t game, const std::uint32_t state,
        const std::uint32_t argument) override {
        calls.push_back({"initialize-state", game, state, argument});
        return state_result;
    }
};

void OpenShopAndRejectInvalidGoods() {
    std::uint32_t state = 0u;
    pal4::TradeSystemUi ui(state);
    Runtime runtime;
    Check(ui.OpenShop(0x1234u, 7u, 3u, runtime) == 1u && state == 1u,
          "valid trade opens shop and marks active state");
    Check(runtime.calls == std::vector<Call>{
              {"music-text"}, {"music-player"}, {"fade", 0x7000u},
              {"music-text-destroy"}, {"database"},
              {"find-goods", 0x906cu, 0x1234u},
              {"ui-frame"}, {"event", 0x8104u, 0x8b0a44u, 1u},
              {"ui-frame"}, {"event", 0x8104u, 0x8b0a4cu, 7u},
              {"ui-frame"}, {"event", 0x8104u, 0x8b0a48u, 3u, 0xa000u},
              {"ui-frame"}, {"toggle", 0x8000u, 1u}},
          "trade open preserves music, lookup and UI event sequence");

    Runtime no_goods;
    no_goods.goods = 0u;
    state = 0u;
    Check(ui.OpenShop(0x1234u, 1u, 0u, no_goods) == 0u && state == 1u &&
              no_goods.calls.size() == 6u,
          "missing shop goods skips events after music and state update");
    Runtime invalid_group;
    state = 0u;
    Check(ui.OpenShop(0x1234u, 1u, 4u, invalid_group) == 0u &&
              state == 1u && invalid_group.calls.size() == 6u,
          "group indices four and above skip UI events");
}

void CloseShopRestoresArenaMusic() {
    std::uint32_t state = 1u;
    pal4::TradeSystemUi ui(state);
    Runtime runtime;
    Check(ui.CloseShop(runtime) == -11 && state == 0u,
          "closing shop forwards music result and clears state");
    Check(runtime.calls == std::vector<Call>{
              {"ui-frame"}, {"event", 0x8104u, 0x8b0a44u},
              {"ui-frame"}, {"toggle", 0x8000u},
              {"arena"}, {"read", 0xb0e0u},
              {"arena"}, {"read", 0xb008u},
              {"music-player"}, {"set-music", 0x7000u, 0xc000u, 0xd000u}},
          "shop close hides UI before reading both arena music fields");
}

void TradeItemInfoAndState() {
    std::uint32_t state = 9u;
    pal4::TradeSystemUi ui(state);
    Runtime setup;
    Check(ui.SetTradeItemInfo(0x1122u, 0x3344u, setup) == 0x34u &&
              state == 0u,
          "trade item setup forwards low event result and clears state");
    Check(setup.calls == std::vector<Call>{
              {"game-state"}, {"initialize-state", 0xe000u, 4u},
              {"ui-frame"}, {"event", 0x8104u, 0x8b0984u, 1u, 0x1122u},
              {"ui-frame"}, {"event", 0x8104u, 0x8b0988u, 1u, 0x3344u},
              {"ui-frame"}, {"event", 0x8104u, 0x8b0980u, 1u}},
          "trade setup enters state four and emits three ordered events");

    Runtime clear;
    state = 9u;
    Check(ui.ClearTradeItemInfo(clear) == 0x34u && state == 0u &&
              clear.calls == std::vector<Call>{
                  {"game-state"}, {"initialize-state", 0xe000u, 4u},
                  {"ui-frame"}, {"event", 0x8104u, 0x8b0980u, 1u}},
          "clear trade info enters state four and shows empty selector");

    Runtime trade_state;
    Check(ui.SetTradeState(trade_state) == -17 && state == 2u &&
              trade_state.calls == std::vector<Call>{
                  {"game-state"}, {"initialize-state", 0xe000u, 1u}},
          "trade state transition returns state initializer result");
}

}  // namespace

int main() {
    try {
        OpenShopAndRejectInvalidGoods();
        CloseShopRestoresArenaMusic();
        TradeItemInfoAndState();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
