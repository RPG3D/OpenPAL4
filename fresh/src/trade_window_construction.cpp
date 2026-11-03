#include "pal4/trade_window.h"
#include <array>
#include <string>
namespace pal4 {
namespace {
struct NamedWindow {
    std::uint32_t offset;
    std::string_view name;
    bool exact = false;
};

constexpr std::array kWindows{
    NamedWindow{0x2cu, "TradeWindow/EditDescription"},
    NamedWindow{0x30u, "TradeWindow/EditProp"},
    NamedWindow{0x34u, "TradeWindow/StaticMoney"},
    NamedWindow{0x38u, "TradeWindow/StaticNeedMoney"},
    NamedWindow{0x3cu, "TradeWindow/BtnOk"},
    NamedWindow{0x40u, "TradeWindow/BtnCancel"},
    NamedWindow{0x44u, "TradeWindow/BtnLeftArrow"},
    NamedWindow{0x48u, "TradeWindow/BtnRightArrow"},
    NamedWindow{0x50u, "TradeWindow/StaticGoodsType"},
    NamedWindow{0xd4u, "TradeWindow/WindowRoleState"},
    NamedWindow{0xd8u, "TradeWindow/daoju", true},
    NamedWindow{0xdcu, "TradeWindow/StaticBuy"},
    NamedWindow{0xe0u, "TradeWindow/StaticSell"},
    NamedWindow{0xe4u, "TradeWindow/ImgGoodsType"},
};

constexpr std::array<std::string_view, 8> kRoleNames{
    "TradeWindow/ImageRole", "TradeWindow/Icon",
    "TradeWindow/StaticRoleWu", "TradeWindow/StaticRoleFang",
    "TradeWindow/StaticRoleSu", "TradeWindow/StaticRoleYun",
    "TradeWindow/StaticRoleLing", "TradeWindow/StaticLevel",
};

struct FrameEvent {
    std::uint32_t key;
    std::uint32_t handler;
};

constexpr std::array kFrameEvents{
    FrameEvent{0x8b0a48u, 0x48bea0u},
    FrameEvent{0x8b0a4cu, 0x48c5c0u},
    FrameEvent{0x8b185cu, 0x48cc20u},
    FrameEvent{0x8b1860u, 0x48cc80u},
    FrameEvent{0x8b1864u, 0x48cce0u},
};

struct UiSubscription {
    std::uint32_t offset;
    TradeUiEvent event;
    std::uint32_t handler;
};

constexpr std::array kSubscriptions{
    UiSubscription{0x28u, TradeUiEvent::HoverRowChanged, 0x48d1d0u},
    UiSubscription{0x28u, TradeUiEvent::MouseClick, 0x48d1a0u},
    UiSubscription{0x44u, TradeUiEvent::ButtonClicked, 0x48d290u},
    UiSubscription{0x48u, TradeUiEvent::ButtonClicked, 0x48d310u},
    UiSubscription{0x3cu, TradeUiEvent::ButtonClicked, 0x48d390u},
    UiSubscription{0x40u, TradeUiEvent::ButtonClicked, 0x48d680u},
    UiSubscription{0u, TradeUiEvent::ButtonClicked, 0x48d680u},
    UiSubscription{0x4cu, TradeUiEvent::ButtonClicked, 0x48d710u},
};

constexpr std::array<std::string_view, 15> kImages{
    "wuqi", "toubu", "shenti", "zhubu", "peidai", "xinzeng",
    "zi", "zi1", "zi2", "zi3", "zi4", "xinzeng", "rongzhu",
    "xinzeng", "duanye",
};

void InitializeText(const std::uint32_t address, ITradeWindowConstructionRuntime& runtime) {
    runtime.WriteByte(address, 0u);
    runtime.WriteWord(address + 4u, 0u);
    runtime.WriteWord(address + 8u, 0u);
    runtime.WriteWord(address + 12u, 0u);
}

void AddFrameEvent(const std::uint32_t frame, const FrameEvent event,
                   ITradeWindowConstructionRuntime& runtime) {
    const auto callback = runtime.Allocate(12u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, event.handler);
        runtime.WriteWord(callback + 8u, frame);
        runtime.WriteWord(callback, TradeWindow::kFrameCallbackVtable);
    }
    try {
        runtime.InsertOrReplace(runtime.FrameManager() + 0x104u,
                                event.key, callback);
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback);
        throw;
    }
    if (callback != 0u) runtime.ReleaseCallback(callback);
}

void AddSubscription(const std::uint32_t frame, const std::uint32_t window,
                     const TradeUiEvent event, const std::uint32_t handler,
                     ITradeWindowConstructionRuntime& runtime) {
    const auto callback = runtime.Allocate(12u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, handler);
        runtime.WriteWord(callback + 8u, frame);
        runtime.WriteWord(callback, TradeWindow::kUiCallbackVtable);
    }
    std::uint32_t connection = 0u;
    try {
        connection = runtime.Subscribe(window, event, callback);
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback);
        throw;
    }
    if (connection != 0u) runtime.ReleaseConnection(connection);
}

}  // namespace
std::uint32_t TradeWindow::Construct(const std::uint32_t frame,
                                     const std::uint32_t name,
                                     ITradeWindowConstructionRuntime& runtime) {
    runtime.LoadLayout(frame, name);
    unsigned stage = 0;
    try {
        runtime.WriteWord(frame + 0xecu, 0u);
        runtime.WriteWord(frame + 0xf0u, 0u);
        runtime.WriteByte(frame + 0xf8u, 1u);
        InitializeText(frame + 0xfcu, runtime);
        stage = 1;
        runtime.AssignText(frame + 0xfcu, "updateTradeItem");
        InitializeText(frame + 0x10cu, runtime);
        stage = 2;
        runtime.AssignText(frame + 0x10cu, "regUpdateEvent");
        for (const auto offset : {0x11cu, 0x120u, 0x124u}) {
            runtime.WriteWord(frame + offset, 0u);
        }
        runtime.ConstructNameNode(frame + 0x128u);
        stage = 3;
        for (const auto offset : {0x130u, 0x140u, 0x150u}) {
            InitializeText(frame + offset, runtime);
            ++stage;
        }
        runtime.WriteWord(frame + 0x19cu, 0x843d68u);
        runtime.WriteWord(frame + 0x1a8u, 0x843d50u);
        runtime.WriteWord(frame, kVtable);

        for (const auto event : kFrameEvents) AddFrameEvent(frame, event, runtime);

        const auto list = runtime.FindWindow("TradeWindow/List");
        runtime.WriteWord(frame + 0x28u, list);
        runtime.SetSortDirection(list, 0u);
        runtime.SetColumnDraggingEnabled(list, false);
        runtime.SetColumnSizingEnabled(list, false);
        runtime.SetUserSortEnabled(list, false);
        runtime.ResetList(list);
        for (auto remaining = runtime.ColumnCount(list); remaining != 0u;
             --remaining) {
            runtime.RemoveFirstColumn(list);
        }
        for (const auto& window : kWindows) {
            const auto value = window.exact ? runtime.GetWindow(window.name)
                                            : runtime.FindWindow(window.name);
            runtime.WriteWord(frame + window.offset, value);
            if (window.offset == 0x34u || window.offset == 0x38u) {
                runtime.SetHorizontalFormatting(value, 1u);
            }
        }
        const auto close_button = runtime.FindWindow("TradeWindow/BtnClose");
        runtime.WriteWord(frame + 0x4cu,
                          runtime.FindWindow("TradeWindow/BtnHelp"));
        for (std::uint32_t role = 1; role <= 4; ++role) {
            for (std::uint32_t item = 0; item < kRoleNames.size(); ++item) {
                const auto window = runtime.FindWindow(
                    std::string(kRoleNames[item]) + std::to_string(role));
                runtime.WriteWord(frame + 0x54u + (role - 1u) * 0x20u +
                                      item * 4u, window);
                if (item == 7u) {
                    runtime.SetFont(window, "Level_font");
                    runtime.SetFormatting(window, 3u, 2u);
                }
            }
        }
        for (std::uint32_t i = 0; i < kSubscriptions.size(); ++i) {
            const auto& subscription = kSubscriptions[i];
            const auto window = i == 6u ? close_button
                                         : runtime.ReadWord(frame + subscription.offset);
            AddSubscription(frame, window, subscription.event,
                            subscription.handler, runtime);
        }
        for (std::uint32_t i = 0; i < kImages.size(); ++i) {
            runtime.WriteWord(frame + 0x160u + i * 4u,
                              runtime.ResolveImage("zi0", kImages[i]));
        }
        runtime.WriteWord(frame + 0x1a0u, frame);
        runtime.WriteWord(frame + 0x1acu, frame);
    } catch (...) {
        if (stage >= 6) runtime.DestroyText(frame + 0x150u);
        if (stage >= 5) runtime.DestroyText(frame + 0x140u);
        if (stage >= 4) runtime.DestroyText(frame + 0x130u);
        if (stage >= 3) runtime.DestroyNameNode(frame + 0x128u);
        if (stage >= 2) runtime.DestroyText(frame + 0x10cu);
        if (stage >= 1) runtime.DestroyText(frame + 0xfcu);
        static_cast<void>(runtime.DestroyBase(frame));
        throw;
    }
    return frame;
}

}  // namespace pal4
