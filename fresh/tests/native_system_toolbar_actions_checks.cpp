#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_system_toolbar_actions.h"
#include "pal4/native_cegui_window_pointer_events.h"
#include "pal4/native_system_toolbar_visibility.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/player_manager_object.h"
#include "pal4/system_toolbar_visibility.h"
#include "support/native_desktop_observation.h"
#include "../native/ui/cegui_exports_native.h"

#include <tuple>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) {
        // Match the enclosing native fixture's Check: stop immediately rather
        // than unwind through an active RenderWare frame during test cleanup.
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

[[noreturn]] void Unexpected() { throw std::logic_error("unused toolbar game service requested"); }

// Game page-event consumers remain probes. The source EventArgs, names,
// seven ButtonBase objects, subscriptions and selected state are original DLL
// objects; no CEGUI calls are simulated by this delegate.
class GameServices final : public pal4::ISystemToolbarOperationsRuntime {
public:
    std::array<std::uint32_t, 80> manager{};
    std::uint8_t smith = 0;
    std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>> events;
    std::function<void(std::uint32_t, std::uint32_t, std::uint32_t)> event_sink;
    std::uint32_t ReadWord(std::uint32_t) override { Unexpected(); }
    void WriteByte(std::uint32_t, std::uint8_t) override { Unexpected(); }
    bool CompareWindowName(std::uint32_t, std::string_view) override { Unexpected(); }
    std::uint32_t FrameManager() override { return reinterpret_cast<std::uint32_t>(manager.data()); }
    void DispatchEvent(std::uint32_t tree, std::uint32_t key,
                       std::uint32_t first, std::uint32_t second) override {
        Require(tree == FrameManager() + 260u, "toolbar dispatch uses owned manager event area");
        if (key == 0x8b0f1cu && first)
            Require(smith == 1u, "smith flag precedes forging show event");
        events.emplace_back(key, first, second);
        if (event_sink) event_sink(key, first, second);
    }
    void SetSelected(std::uint32_t, bool) override { Unexpected(); }
    void SetCurrentSelected(std::uint32_t, std::uint32_t) override { Unexpected(); }
    std::uint32_t SmithSystem() override { return reinterpret_cast<std::uint32_t>(&smith); }
    void SetVisible(std::uint32_t, std::uint32_t, std::uint32_t) override { Unexpected(); }
    std::uint32_t Game() override { Unexpected(); }
    std::int32_t SceneMode(std::uint32_t) override { Unexpected(); }
    std::uint32_t TradeSystem() override { Unexpected(); }
    std::uint8_t ClearTradeItemInfo(std::uint32_t) override { Unexpected(); }
    std::uint8_t InitializeGameState(std::uint32_t, std::uint32_t, std::uint32_t) override { Unexpected(); }
    void HandleMouseEvent(std::uint32_t) override { Unexpected(); }
};
}

void VerifyNativeSystemToolbarActions(pal4::NativeCeguiUiSession& ui) {
    using pal4::detail::Export;
    const auto module = GetModuleHandleA("CEGUIBase.dll");
    GameServices game;
    pal4::NativeSystemToolbarActions actions(module, ui, game);
    struct Clear {
        pal4::NativeCeguiUiSession& ui;
        ~Clear() { ui.ClearSystemToolbarClickHandler(); }
    } clear{ui};
    unsigned clicks = 0;
    auto handler = [&](std::uint32_t event) {
        ++clicks;
        const bool handled = actions.OnButtonClick(event);
        std::uint32_t source = 0;
        std::memcpy(&source, reinterpret_cast<const void*>(event + 8u), 4u);
        for (const auto button : ui.SystemToolbarButtons())
            Require(*reinterpret_cast<const std::uint8_t*>(button + 1201u) ==
                        (button == source ? 1u : 0u),
                    "shared navigation sets native highlight during the click handler");
        return handled;
    };
    Require(ui.BindSystemToolbarClickHandler(handler) &&
                ui.BindSystemToolbarClickHandler(handler),
            "toolbar click handler binds repeatedly without duplicate subscriptions");
    const auto clicked = Export<const void*>(module,
        "?EventClicked@PushButton@CEGUI@@2VString@2@B");
    using Fire = void (__thiscall*)(void*, const void*, void*, const void*);
    const auto fire = Export<Fire>(module,
        "?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    const auto construct = Export<void (__thiscall*)(void*, void*)>(module,
        "??0WindowEventArgs@CEGUI@@QAE@PAVWindow@1@@Z");
    const auto destroy = Export<void (__thiscall*)(void*)>(module,
        "??1WindowEventArgs@CEGUI@@UAE@XZ");
    const auto selected = Export<bool (__thiscall*)(const void*)>(module,
        "?isSelected@ButtonBase@CEGUI@@QBE_NXZ");
    pal4::detail::NativeString empty(module, "");
    auto fire_click = [&](std::uint32_t button) {
        // EventArgs vptr, handled flag, source Window*. Construct/destruct using
        // the DLL so this also checks the original EventSet subscriber ABI.
        alignas(4) std::array<std::byte, 12> event{};
        construct(event.data(), reinterpret_cast<void*>(button));
        fire(reinterpret_cast<void*>(button), clicked, event.data(), empty.Data());
        destroy(event.data());
    };
    constexpr std::array<std::uint32_t, 7> keys{
        0x8a80bcu, 0x8a81d0u, 0x8b00d4u, 0x8a8098u,
        0x8a8548u, 0x8b06a8u, 0x8b0f1cu};
    const auto& buttons = ui.SystemToolbarButtons();
    std::uint32_t context = 0;
    std::memcpy(&context, reinterpret_cast<const void*>(ui.SystemToolbarRootAddress() + 68u), 4u);
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        game.events.clear();
        game.smith = 0;
        fire_click(buttons[i]);
        ui.RethrowSystemToolbarClickError();
        Require(clicks == i + 1u && game.events.size() == 8u,
                "each native toolbar click reaches shared operations exactly once");
        for (std::size_t j = 0; j < keys.size(); ++j) {
            Require(game.events[j] == std::tuple{keys[j], 0u, 0u},
                    "native toolbar click clears all seven pages in original order");
            Require(selected(reinterpret_cast<const void*>(buttons[j])) == (i == j),
                    "original DLL has exactly the clicked toolbar button selected");
            Require(*reinterpret_cast<const std::uint8_t*>(buttons[j] + 1201u) == (i == j ? 1u : 0u),
                    "shared navigation selects the real native highlight byte exclusively");
        }
        Require(game.events.back() == std::tuple{keys[i], 1u, context} &&
                    game.smith == (i == 6u ? 1u : 0u),
                "toolbar show payload and forging flag match IDA");
    }
    actions.ClearAllSelection();
    for (const auto button : buttons)
        Require(!selected(reinterpret_cast<const void*>(button)), "toolbar clear resets native selection");
    pal4::NativeCeguiWindowPointerEvents pointer(module);
    struct Rect { float top, bottom, left, right; };
    using GetRect = Rect* (__thiscall*)(const void*, Rect*);
    const auto get_rect = Export<GetRect>(module,
        "?getUnclippedPixelRect@Window@CEGUI@@QBE?AVRect@2@XZ");
    for (const auto button : buttons) {
        Rect rect;
        get_rect(reinterpret_cast<const void*>(button), &rect);
        const auto origin = pointer.AbsolutePosition(button);
        Require(std::abs(origin.x - rect.left) < 0.01f &&
                    std::abs(origin.y - rect.top) < 0.01f,
                "shared pointer coordinates match the original DLL pixel rectangle");
    }
    const pal4::test::NativeDesktopObservation shortcut_desktop;
    Require(ui.SetSystemToolbarLayoutVisible(true), "show toolbar for native shortcut events");
    struct Hide {
        pal4::NativeCeguiUiSession& ui;
        ~Hide() { (void)ui.SetSystemToolbarLayoutVisible(false); }
    } hide{ui};
    constexpr std::array<std::size_t, 7> shortcut_order{2u, 0u, 3u, 1u, 6u, 4u, 5u};
    std::array<std::uint32_t, 5> key_event{};
    for (std::size_t i = 0; i < shortcut_order.size(); ++i) {
        const auto target = shortcut_order[i];
        key_event[4] = 59u + static_cast<std::uint32_t>(i);
        game.events.clear();
        game.smith = 0;
        Require(actions.HandleKeyInput(reinterpret_cast<std::uint32_t>(key_event.data())) &&
                    clicks == 8u + i && game.events.size() == 8u &&
                    game.events.back() == std::tuple{keys[target], 1u, context},
                "F1-F7 shared shortcuts click the original DLL buttons in original order");
        for (std::size_t j = 0; j < buttons.size(); ++j) {
            Require(selected(reinterpret_cast<const void*>(buttons[j])) == (j == target),
                    "toolbar shortcut selects its real native button");
        }
    }
    for (const auto key : {58u, 66u}) {
        key_event[4] = key;
        Require(!actions.HandleKeyInput(reinterpret_cast<std::uint32_t>(key_event.data())) && clicks == 14u,
                "neighboring unsupported toolbar keys do not click");
    }
    for (std::size_t i = 0; i < shortcut_order.size(); ++i) {
        const auto target = shortcut_order[i];
        game.events.clear();
        game.smith = 0;
        Require(actions.HandleSystemEvent(0x8a80b8u,
                    {static_cast<std::uint32_t>(i + 1u), 0x1234u}) == 1u &&
                    clicks == 15u + i && game.events.size() == 8u &&
                    game.events.back() == std::tuple{keys[target], 1u, context},
                "system page command reuses native centered pointer clicks in original order");
        for (std::size_t j = 0; j < buttons.size(); ++j)
            Require(selected(reinterpret_cast<const void*>(buttons[j])) == (j == target),
                    "system page command selects exactly its original DLL button");
    }
    game.events.clear();
    for (const auto choice : {0u, 8u, 0xffffffffu})
        Require(actions.HandleSystemEvent(0x8a80b8u, {choice, 0u}) == 1u &&
                    clicks == 21u && game.events.empty(),
                "invalid system page choices are handled without clicks or events");
    Require(actions.HandleSystemEvent(0x8a80b0u, {0u, 0u}) == 1u &&
                game.events.size() == 8u &&
                game.events.back() == std::tuple{0x8a80acu, 1u, 0u},
            "system entry reset clears pages before requesting toolbar visibility");
    for (std::size_t i = 0; i < buttons.size(); ++i)
        Require(game.events[i] == std::tuple{keys[i], 0u, 0u} &&
                    !selected(reinterpret_cast<const void*>(buttons[i])),
                "system entry reset clears every page and real DLL selected state");
    bool foreign_descriptor = false;
    try { (void)actions.HandleSystemEvent(0x8a80a4u, {0u, 0u}); }
    catch (const std::invalid_argument&) { foreign_descriptor = true; }
    Require(foreign_descriptor && clicks == 21u && game.events.size() == 8u,
            "toolbar command adapter rejects scene-owned descriptors without side effects");
    shortcut_desktop.Check("native shortcut synthesis leaves OS cursor and foreground unchanged");
    actions.ClearAllSelection();
    Require(ui.SetSystemToolbarLayoutVisible(false), "hide toolbar after shortcut check");
    Require(ui.BindSystemToolbarClickHandler([](std::uint32_t) -> bool {
        throw std::runtime_error("toolbar callback exception probe");
    }), "toolbar callback replacement succeeds");
    fire_click(buttons[0]);
    bool propagated = false;
    try { ui.RethrowSystemToolbarClickError(); }
    catch (const std::runtime_error& error) {
        propagated = std::string_view(error.what()) == "toolbar callback exception probe";
    }
    Require(propagated, "toolbar callback error propagates outside the DLL subscriber ABI");
    ui.RethrowSystemToolbarClickError();
    ui.ClearSystemToolbarClickHandler();
    fire_click(buttons[0]);
    Require(clicks == 21u, "cleared toolbar callback no longer borrows its controller");
    std::cout << "Native toolbar DLL clicks, shared actions, seven selections and callback lifetime verified\n";
    std::cout << "Native toolbar system-entry reset and seven page commands verified\n";
    std::cout << "Native toolbar F1-F7 pointer synthesis and original DLL geometry verified\n";
    std::cout << "Native toolbar shared navigation selection and redraw verified\n";
}

void VerifyNativeSystemToolbarVisibility(pal4::NativeCeguiUiSession& ui,
    pal4::NativePalGameIVTextureStorage& live_game, pal4::PlayerManagerObjectState& players,
    pal4::NativeEventHandlerArray& active,
    std::function<void(pal4::NativeSystemToolbarActions*)> bind_controller, HWND window) {
    using pal4::detail::Export;
    const auto module = GetModuleHandleA("CEGUIBase.dll");
    GameServices game;
    pal4::NativeSystemToolbarActions actions(module, ui, game);
    const auto original_size = active.Size();
    const auto original_last = active.GetLast();
    bind_controller(&actions);
    struct Unbind {
        decltype(bind_controller)& callback;
        ~Unbind() { callback(nullptr); }
    } unbind{bind_controller};
    pal4::NativeSystemToolbarVisibilityServices services{live_game, players, active};
    const auto toolbar = ui.SystemToolbarRootAddress();
    const auto buttons = ui.SystemToolbarButtons();
    const auto get_parent = Export<void* (__thiscall*)(const void*)>(module,
        "?getParent@Window@CEGUI@@QBEPAV12@XZ");
    const auto disabled = Export<bool (__thiscall*)(const void*, bool)>(module,
        "?isDisabled@Window@CEGUI@@QBE_N_N@Z");
    const auto set_enabled = Export<void (__thiscall*)(void*, bool)>(module,
        "?setEnabled@Window@CEGUI@@QAEX_N@Z");
    auto* original_parent = get_parent(reinterpret_cast<const void*>(toolbar));
    Require(original_parent != nullptr, "toolbar initially belongs to live desktop");
    constexpr std::array<std::uint32_t, 6> entry_keys{8u, 9u, 10u, 11u, 13u, 14u};
    constexpr std::array<std::size_t, 6> button_order{2u, 0u, 3u, 1u, 4u, 5u};
    std::array<std::uint32_t, 6> saved_entries{};
    std::array<bool, 7> saved_disabled{};
    std::array<std::uint8_t, 7> saved_highlights{};
    for (std::size_t i = 0; i < entry_keys.size(); ++i)
        saved_entries[i] = live_game.GameDataEntry(entry_keys[i]);
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        saved_disabled[i] = disabled(reinterpret_cast<const void*>(buttons[i]), true);
        saved_highlights[i] = *reinterpret_cast<const std::uint8_t*>(buttons[i] + 1201u);
    }
    auto& forge_flag = players.Word((3u * 0xb14u + 0xb08u) / 4u);
    const auto saved_forge = forge_flag;
    const auto parent_event = pal4::SystemToolbarVisibility::kParentEvent;
    const auto window_event = pal4::SystemToolbarVisibility::kWindowEvent;
    const auto show_event = pal4::SystemToolbarVisibility::kShowEvent;
    game.event_sink = [&](std::uint32_t key, std::uint32_t enabled, std::uint32_t parent) {
        if (key == 0x8a80acu)
            Require(actions.HandleVisibilityEvent(enabled, parent, services),
                    "toolbar show event reaches the real preloaded frame");
    };
    // Exercise the entry callback's nested show event against the actual main
    // activity array, before the direct raw-byte visibility regressions.
    (void)actions.UpdateVisibility(0u, 0u, services);
    game.events.clear();
    Require(actions.HandleSystemEvent(0x8a80b0u, {0u, 0u}) == 1u &&
                active.Size() == original_size + 1u &&
                active.GetLast() == actions.ActiveComponentAddress() &&
                get_parent(reinterpret_cast<const void*>(toolbar)) == original_parent &&
                game.events.size() == 11u &&
                game.events[7] == std::tuple{0x8a80acu, 1u, 0u} &&
                game.events[8] == std::tuple{parent_event, 1u, 0u} &&
                game.events[9] == std::tuple{window_event, 1u, toolbar} &&
                game.events[10] == std::tuple{show_event, 5u, 0u},
            "system entry reset composes real toolbar attachment and main-array activity");
    game.events.clear();
    Require(actions.HandleVisibilityEvent(0x100u, 0u, services) &&
                active.Size() == original_size + 1u &&
                game.events == decltype(game.events){
                    {parent_event, 1u, 0u}, {window_event, 1u, toolbar}, {show_event, 5u, 0u}},
            "generic toolbar show uses the full nonzero word before normalizing to one");
    game.events.clear();
    Require(actions.HandleVisibilityEvent(0u, 0u, services) &&
                !get_parent(reinterpret_cast<const void*>(toolbar)) &&
                active.Size() == original_size && active.GetLast() == original_last,
            "generic toolbar hide detaches and restores the preceding main activity");
    for (unsigned pass = 0u; pass < 2u; ++pass) {
        for (std::size_t i = 0; i < entry_keys.size(); ++i)
            live_game.SetGameDataEntry(entry_keys[i], (i + pass) % 2u);
        forge_flag = pass;
        game.events.clear();
        (void)actions.UpdateVisibility(7u, 0u, services);
        Require(active.Size() == original_size + 1u && active.GetLast() == actions.ActiveComponentAddress() &&
                    get_parent(reinterpret_cast<const void*>(toolbar)) == original_parent,
                "toolbar show attaches parent and inserts real activity only once");
        Require(game.events == decltype(game.events){
                    {parent_event, 7u, 0u}, {window_event, 7u, toolbar}, {show_event, 5u, 0u}},
                "toolbar visibility preserves exact original event order and visible byte");
        for (std::size_t i = 0; i < entry_keys.size(); ++i)
            Require(disabled(reinterpret_cast<const void*>(buttons[button_order[i]]), true) ==
                        (((i + pass) % 2u) == 0u),
                    "toolbar enables follow six live game entries");
        Require(disabled(reinterpret_cast<const void*>(buttons[6]), true) == (pass == 0u),
                "toolbar forging enable follows live fourth player flag");
    }
    for (const auto key : entry_keys) live_game.SetGameDataEntry(key, 1u);
    forge_flag = 1u;
    (void)actions.UpdateVisibility(1u, 0u, services);
    active.Update();
    for (const auto button : buttons)
        Require(*reinterpret_cast<const std::uint8_t*>(button + 1201u) == 0u,
                "toolbar active update calls shared redraw-all on native windows");
    unsigned clicks = 0;
    Require(ui.BindSystemToolbarClickHandler([&](std::uint32_t event) {
        ++clicks;
        return actions.OnButtonClick(event);
    }), "toolbar activity check binds native click actions");
    const auto construct = Export<void (__thiscall*)(void*, void*)>(module,
        "??0KeyEventArgs@CEGUI@@QAE@PAVWindow@1@@Z");
    const auto destroy = Export<void (__thiscall*)(void*)>(module,
        "??1KeyEventArgs@CEGUI@@UAE@XZ");
    alignas(4) std::array<std::byte, 24> event{};
    construct(event.data(), reinterpret_cast<void*>(toolbar));
    const auto event_address = reinterpret_cast<std::uint32_t>(event.data());
    auto set_key = [&](std::uint32_t key) { std::memcpy(event.data() + 16u, &key, 4u); };
    for (std::uint32_t key = 59u; key <= 65u; ++key) {
        const auto vk = VK_F1 + key - 59u;
        (void)ui.ProcessKeyboardMessage(window, WM_KEYDOWN, vk, 0u);
        Require(clicks == key - 59u, "toolbar F1-F7 presses do not trigger clicks");
        (void)ui.ProcessKeyboardMessage(window, WM_KEYUP, vk, 0u);
        Require(clicks == key - 58u,
                "permanent desktop/global/active-array route sends F1-F7 releases to native DLL clicks");
    }
    set_key(205u);
    Require(active.DispatchKeyDown(active.GetLast(), event_address) == 1u &&
                *reinterpret_cast<const std::uint32_t*>(actions.FrameAddress() + 60u) == 1u,
            "toolbar activity key-down cycles original ordered navigation");
    set_key(28u);
    Require(active.DispatchKeyDown(active.GetLast(), event_address) == 1u && clicks == 8u,
            "toolbar activity Enter presses click the current original button");
    set_key(17u);
    Require(active.DispatchKeyDown(active.GetLast(), event_address) == 0u &&
                actions.DispatchActivePredicate(actions.ActiveComponentAddress(), 8u, event_address) == 0u &&
                clicks == 8u,
            "unsupported activity key remains unhandled");
    destroy(event.data());
    game.events.clear();
    (void)actions.UpdateVisibility(0u, 0u, services);
    Require(!get_parent(reinterpret_cast<const void*>(toolbar)) && active.Size() == original_size &&
                active.GetLast() == original_last &&
                game.events == decltype(game.events){{parent_event, 0u, 0u}, {window_event, 0u, toolbar}},
            "toolbar hide detaches parent, dispatches original events and removes activity");
    ui.ClearSystemToolbarClickHandler();
    game.event_sink = {};
    for (std::size_t i = 0; i < saved_entries.size(); ++i)
        live_game.SetGameDataEntry(entry_keys[i], saved_entries[i]);
    forge_flag = saved_forge;
    for (std::size_t i = 0; i < buttons.size(); ++i) {
        set_enabled(reinterpret_cast<void*>(buttons[i]), !saved_disabled[i]);
        *reinterpret_cast<std::uint8_t*>(buttons[i] + 1201u) = saved_highlights[i];
    }
    Require(ui.BindSystemToolbarParent(true, reinterpret_cast<std::uint32_t>(original_parent)) &&
                ui.SetSystemToolbarLayoutVisible(false), "restore hidden toolbar resource state");
    std::cout << "Native toolbar visibility, live game/player enables and active-array key routing verified\n";
    std::cout << "Native system reset to toolbar visibility and main activity composition verified\n";
}
