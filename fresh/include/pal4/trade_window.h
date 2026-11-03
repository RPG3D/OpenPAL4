#pragma once

#include "pal4/trade_window_construction.h"
#include "pal4/trade_window_data.h"
#include "pal4/trade_window_repeat.h"
#include "pal4/trade_window_lifecycle.h"
#include <cstdint>
#include <array>
#include <string_view>
#include <utility>

namespace pal4 {

// Gameplay lifecycle/data services extend the independently usable layout boundary.
class ITradeWindowRuntime : public ITradeWindowConstructionRuntime,public ITradeWindowDataRuntime,public ITradeWindowPressRuntime,public ITradeWindowLifecycleRuntime {
public:
    ~ITradeWindowRuntime() override = default;
    virtual std::uint32_t ReadWord(std::uint32_t) override=0;
    virtual void WriteWord(std::uint32_t,std::uint32_t) override=0;
    virtual std::uint8_t ReadByte(std::uint32_t) override=0;
    virtual void WriteByte(std::uint32_t,std::uint8_t) override=0;
    virtual std::uint32_t FrameManager() override=0;

    virtual void SetParent(std::uint32_t frame, bool visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::pair<std::uint32_t, std::uint32_t> CreateEffect(
        std::uint32_t manager, std::string_view name,
        const std::array<float, 3>& position,
        std::uint32_t selector, bool flag) = 0;
    virtual void ResetTemporaryEffect(std::uint32_t first,
                                      std::uint32_t second) = 0;
    virtual void SetEffectProperties(std::uint32_t handle,
                                     const std::array<float, 3>& scale,
                                     bool flag) = 0;
    virtual void MarkEffectForUpdate(std::uint32_t handle) = 0;
    virtual void AddAndProcessScript() = 0;
    [[nodiscard]] virtual std::uint32_t CreateGameObject(
        std::uint32_t collection, std::string_view path,
        std::string_view name, bool flag) = 0;
    virtual void SetGameObjectScale(std::uint32_t object,
                                    const std::array<float, 3>& scale) = 0;
    virtual void SetGameObjectPosition(
        std::uint32_t object, const std::array<float, 3>& position) = 0;
    virtual void CleanupScripts() = 0;
    virtual void SetGameObjectDisplay(std::uint32_t frame,
                                      std::uint32_t a, std::uint32_t b,
                                      std::uint32_t c) = 0;
    virtual void RemoveGameObject(std::uint32_t collection,
                                  std::uint32_t id) = 0;
    virtual void SetWindowText(std::uint32_t window,
                               std::string_view text) = 0;
    virtual void HandleEffectEvent(std::uint32_t handle,
                                   std::uint32_t event) = 0;
    virtual void RemoveObjectByIdentifier(std::uint32_t collection,
                                          std::uint32_t identifier) = 0;
    virtual void RemoveUiElementByIdentifier(std::uint32_t collection,
                                             std::uint32_t identifier) = 0;
    virtual void AddEventHandler(std::uint32_t collection,
                                 std::uint32_t identifier,
                                 std::uint32_t delay_bits,
                                 std::uint32_t frame,
                                 std::uint32_t handler,
                                 std::uint32_t event_value,
                                 bool flag) = 0;
    [[nodiscard]] virtual std::uint32_t TradeSystem() = 0;
    virtual void ResetTradeSystem(std::uint32_t trade) = 0;
    virtual void ProcessUiEvent(std::uint32_t event_tree,
                                std::uint32_t key,
                                std::uint32_t first,
                                std::uint32_t second) = 0;
};

class TradeWindow final {
public:
    static constexpr std::uint32_t kObjectSize = 0x1b4u;
    static constexpr std::uint32_t kVtable = 0x843d38u;
    static constexpr std::uint32_t kFrameCallbackVtable = 0x843d2cu;
    static constexpr std::uint32_t kUiCallbackVtable = 0x843d24u;

    // IDA 0x48a980.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t frame, std::uint32_t name,
        ITradeWindowConstructionRuntime& runtime);
    // IDA 0x48bb10.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t frame, ITradeWindowRuntime& runtime);
    // IDA 0x48bc30.
    static void SetVisible(std::uint32_t frame, bool visible,
                           std::uint32_t parent,
                           ITradeWindowLifecycleRuntime& runtime);
    // IDA 0x48bea0.
    [[nodiscard]] static bool UpdateTradeItems(
        std::uint32_t frame, std::uint32_t event,
        ITradeWindowDataRuntime& runtime);
    // IDA 0x48c2d0-0x48c5c0.
    [[nodiscard]] static std::uint32_t GetDataPtr(
        std::uint32_t data_field, ITradeWindowDataRuntime& runtime);
    [[nodiscard]] static bool MeetsOpenCondition(
        std::uint32_t item, ITradeWindowDataRuntime& runtime);
    [[nodiscard]] static bool HasMultipleCategories(
        std::uint32_t frame, ITradeWindowDataRuntime& runtime);
    [[nodiscard]] static bool HasTradeItems(
        std::uint32_t frame, ITradeWindowDataRuntime& runtime);
    [[nodiscard]] static bool SetActiveState(
        std::uint32_t frame, std::uint32_t event,
        ITradeWindowDataRuntime& runtime);
    // IDA 0x48cc20-0x48cce0.
    [[nodiscard]] static bool OnQuantityDecreaseDown(
        std::uint32_t frame, std::uint32_t event,
        ITradeWindowPressRuntime& runtime);
    [[nodiscard]] static bool OnQuantityIncreaseDown(
        std::uint32_t frame, std::uint32_t event,
        ITradeWindowPressRuntime& runtime);
    [[nodiscard]] static bool OnQuantityButtonUp(
        std::uint32_t frame, ITradeWindowPressRuntime& runtime);
    // IDA 0x48d680-0x48d710.
    [[nodiscard]] static bool OnCloseThunk(
        std::uint32_t frame, ITradeWindowLifecycleRuntime& runtime);
    [[nodiscard]] static bool OnClose(
        std::uint32_t frame, ITradeWindowLifecycleRuntime& runtime);
    [[nodiscard]] static bool GlobalUpdateHandler(
        ITradeWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
