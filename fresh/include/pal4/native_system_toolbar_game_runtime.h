#pragma once

#include "pal4/system_toolbar_operations.h"
#include <functional>

namespace pal4 {
class NativeArenaLoadRuntime;
class NativePalGameIVTextureStorage;
class NativePalGameIVStateList;

// Borrowed application endpoints. Events include page hide/show and toolbar
// decoration consumers; a host must provide them rather than consume silently.
struct NativeSystemToolbarGameServices {
    std::function<void(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)> event;
    std::function<void(std::uint32_t, std::uint32_t, std::uint32_t)> visibility;
    std::uint8_t* smith_flag = nullptr;
    std::uint32_t trade = 0u;
    std::function<std::uint8_t(std::uint32_t)> clear_trade;
};

// Registered state objects are resolved through their native owner, never by
// invoking original EXE vtables. Return-to-Scene uses the existing state engine.
class NativeSystemToolbarGameRuntime final : public ISystemToolbarGameRuntime {
public:
    NativeSystemToolbarGameRuntime(NativeArenaLoadRuntime& transitions,
        NativePalGameIVTextureStorage& game, NativePalGameIVStateList& states,
        std::uint32_t manager, NativeSystemToolbarGameServices services);
    std::uint32_t FrameManager() override;
    void DispatchEvent(std::uint32_t tree, std::uint32_t key,
                       std::uint32_t first, std::uint32_t second) override;
    std::uint32_t SmithSystem() override;
    void SetVisible(std::uint32_t frame, std::uint32_t visible, std::uint32_t parent) override;
    std::uint32_t Game() override;
    std::int32_t SceneMode(std::uint32_t state) override;
    std::uint32_t TradeSystem() override;
    std::uint8_t ClearTradeItemInfo(std::uint32_t trade) override;
    std::uint8_t InitializeGameState(std::uint32_t game, std::uint32_t mode,
                                    std::uint32_t argument) override;
private:
    NativeArenaLoadRuntime& transitions_;
    NativePalGameIVTextureStorage& game_;
    NativePalGameIVStateList& states_;
    std::uint32_t manager_;
    NativeSystemToolbarGameServices services_;
};
}  // namespace pal4
