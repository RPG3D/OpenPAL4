#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {
class IUiNavigationInputRuntime;

// Game/state boundary needed by the native DLL toolbar. Window operations are
// supplied by its native controller, independently of these game services.
class ISystemToolbarGameRuntime {
public:
    virtual ~ISystemToolbarGameRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void DispatchEvent(std::uint32_t tree, std::uint32_t key,
                               std::uint32_t first,
                               std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t SmithSystem() = 0;
    virtual void SetVisible(std::uint32_t frame, std::uint32_t visible,
                            std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t Game() = 0;
    [[nodiscard]] virtual std::int32_t SceneMode(std::uint32_t scene) = 0;
    [[nodiscard]] virtual std::uint32_t TradeSystem() = 0;
    [[nodiscard]] virtual std::uint8_t ClearTradeItemInfo(
        std::uint32_t trade) = 0;
    [[nodiscard]] virtual std::uint8_t InitializeGameState(
        std::uint32_t game, std::uint32_t mode,
        std::uint32_t argument) = 0;
};

class ISystemToolbarOperationsRuntime : public ISystemToolbarGameRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual bool CompareWindowName(
        std::uint32_t name_object, std::string_view name) = 0;
    virtual void SetSelected(std::uint32_t button, bool selected) = 0;
    virtual void SetCurrentSelected(std::uint32_t helper, std::uint32_t button) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
};

class SystemToolbarOperations final {
public:
    [[nodiscard]] static std::uint8_t OnButtonClick(
        std::uint32_t frame, std::uint32_t event,
        ISystemToolbarOperationsRuntime& runtime);
    static void ClearAllSelection(
        std::uint32_t frame, ISystemToolbarOperationsRuntime& runtime);
    [[nodiscard]] static std::uint8_t Close(
        std::uint32_t frame, ISystemToolbarOperationsRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleKeyInput(
        std::uint32_t member, std::uint32_t event,
        ISystemToolbarOperationsRuntime& runtime);
    // IDA483480: toolbar's slot0 handles Esc, otherwise navigation keys.
    [[nodiscard]] static std::uint8_t HandleActiveKeyDown(
        std::uint32_t frame, std::uint32_t event,
        ISystemToolbarOperationsRuntime& runtime,
        IUiNavigationInputRuntime& navigation);
};

}  // namespace pal4
