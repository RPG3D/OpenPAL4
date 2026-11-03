#include "pal4/native_common_dialog_layout.h"
#include "pal4/native_cegui_ui_session.h"
#include "../native/ui/cegui_exports_native.h"
#include "support/native_desktop_observation.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cmath>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
}
void VerifyNativeCommonDialogLayout(pal4::NativeCeguiUiSession& ui,
                                   const std::filesystem::path& root) {
    using pal4::detail::Export;
    using pal4::detail::NativeString;
    const auto module = GetModuleHandleA("CEGUIBase.dll");
    const pal4::test::NativeDesktopObservation desktop;
    const auto fire = Export<void (__thiscall*)(void*, const void*, void*, const void*)>(module,
        "?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    NativeString empty(module, "");
    const auto dispatch = [&](std::uint32_t target, bool key, std::uint32_t code = 28u) {
        alignas(4) std::array<std::uint32_t, 5> event{};
        Export<void (__thiscall*)(void*, void*)>(module,
            "??0WindowEventArgs@CEGUI@@QAE@PAVWindow@1@@Z")(event.data(), reinterpret_cast<void*>(target));
        event[4] = code;
        fire(reinterpret_cast<void*>(target), Export<const void*>(module, key ?
            "?EventKeyDown@Window@CEGUI@@2VString@2@B" : "?EventClicked@PushButton@CEGUI@@2VString@2@B"),
            event.data(), empty.Data());
        Export<void (__thiscall*)(void*)>(module,
            "??1WindowEventArgs@CEGUI@@UAE@XZ")(event.data());
    };
    for (unsigned pass = 0; pass < 2u; ++pass) {
        pal4::NativeCommonDialogLayout dialog(module, ui, root);
        Require(dialog.RootAddress() && !Export<bool (__thiscall*)(const void*, bool)>(module,
            "?isVisible@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<void*>(dialog.RootAddress()), true),
            "original CommonDialog layout stages hidden");
        std::vector<std::uint32_t> selections;
        unsigned key_calls = 0u;
        dialog.SetEventHandler([&](bool key, std::uint32_t event) {
            const auto* words = reinterpret_cast<const std::uint32_t*>(event);
            if (key) {
                Require(words[2] == dialog.RootAddress() && words[4] == 28u,
                        "original DLL key subscription preserves event payload");
                ++key_calls;
            } else {
                selections.push_back(reinterpret_cast<std::uint32_t>(Export<void* (__thiscall*)(const void*)>(module,
                    "?getUserData@Window@CEGUI@@QBEPAXXZ")(reinterpret_cast<void*>(words[2]))));
            }
            return true;
        });
        for (const auto* name : {"BtOk", "BtYes", "BtNo"}) dispatch(dialog.Window(name), false);
        dispatch(dialog.RootAddress(), true);
        dialog.RethrowEventError();
        Require(selections == std::vector<std::uint32_t>{4u, 1u, 2u} && key_calls == 1u,
                "shared constructor sets original UserData selection values and installs exactly one subscriber");
        for (const auto* name : {"Message", "FrameMessage", "FrameButton"})
            Require(dialog.Window(name) != 0u, "shared constructor resolves message and frame controls");
        dialog.SetEventHandler([](bool, std::uint32_t) -> bool { throw std::runtime_error("common dialog event probe"); });
        dispatch(dialog.Window("BtYes"), false);
        bool caught = false;
        try { dialog.RethrowEventError(); } catch (const std::runtime_error&) { caught = true; }
        Require(caught, "common dialog subscriber contains exceptions at DLL boundary");
        dialog.RethrowEventError();
        dialog.SetEventHandler({});
        dispatch(dialog.Window("BtYes"), false);
        dialog.RethrowEventError();

        const auto parent = [&] { return Export<void* (__thiscall*)(const void*)>(module,
            "?getParent@Window@CEGUI@@QBEPAV12@XZ")(reinterpret_cast<void*>(dialog.RootAddress())); };
        const auto modal = [&] { return Export<bool (__thiscall*)(const void*)>(module,
            "?getModalState@Window@CEGUI@@QBE_NXZ")(reinterpret_cast<void*>(dialog.RootAddress())); };
        const auto visible = [&](std::string_view name) { return Export<bool (__thiscall*)(const void*, bool)>(module,
            "?isVisible@Window@CEGUI@@QBE_N_N@Z")(reinterpret_cast<void*>(dialog.Window(name)), false); };
        std::vector<std::string> sounds;
        const auto sound = [&](std::string_view file, std::uint32_t repeat) {
            Require(repeat == 1u, "common dialog requests one UI sound");
            sounds.emplace_back(file); // Bounded tests remain muted.
        };
        const auto selection = [&](std::uint32_t value) {
            Require(!modal() && !parent(), "common dialog closes before invoking cloned selection callback");
            selections.push_back(value);
        };
        const auto dimension = [&](std::uint32_t window, const char* name) {
            return Export<float (__thiscall*)(const void*, std::uint32_t)>(module, name)(reinterpret_cast<void*>(window), 1u);
        };
        for (std::uint8_t flags = 0u; flags < 8u; ++flags) {
            dialog.Show(U"\u4f60\u597d\uff0c\u4ed9\u5251\u56db\nConfirm this operation?", flags, selection, sound);
            Require(parent() && modal() && dialog.LastSelection() == 2u &&
                    visible("BtOk") == ((flags & 4u) != 0u) &&
                    visible("BtYes") == ((flags & 1u) != 0u) &&
                    visible("BtNo") == ((flags & 2u) != 0u),
                    "shared show attaches modal root and applies all button flag combinations");
            Require(sounds.back() == ((flags & 4u) ? "wa04.wav" : "wa03.wav"),
                    "shared show preserves original UI sound choice");
            const auto height = Export<float (__thiscall*)(const void*)>(GetModuleHandleA("OIRAMLOOK.dll"),
                "?getTextHeight@OLFormatEditbox@CEGUI@@QBEMXZ")(reinterpret_cast<void*>(dialog.Window("Message")));
            Require(std::abs(dimension(dialog.Window("Message"), "?getHeight@Window@CEGUI@@QBEMW4MetricsMode@2@@Z") - std::round(height + 8.0f)) < 0.01f,
                    "native sizing uses pixel-aligned formatted message height plus original padding");
            const auto get_text = Export<const void* (__thiscall*)(const void*)>(module,
                "?getText@Window@CEGUI@@QBEABVString@2@XZ");
            const auto c_str = Export<const char* (__thiscall*)(const void*)>(module, "?c_str@String@CEGUI@@QBEPBDXZ");
            Require(std::string(c_str(get_text(reinterpret_cast<void*>(dialog.Window("Message"))))) ==
                    "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xbb\x99\xe5\x89\x91\xe5\x9b\x9b\nConfirm this operation?",
                    "formatted native message preserves Unicode text");
            const auto before_escape = selections.size();
            dispatch(dialog.RootAddress(), true, 1u);
            dialog.RethrowEventError();
            const auto expected = (flags & 2u) ? 2u : 4u;
            Require(!parent() && !modal() && dialog.LastSelection() == expected &&
                    selections.size() == before_escape + 1u && selections.back() == expected,
                    "native Escape follows original No-visible versus OK selection and closes first");
        }
        dialog.Show(U"Yes or no?", 3u, selection, sound);
        const auto before_enter = selections.size();
        dispatch(dialog.RootAddress(), true, 205u);
        dispatch(dialog.RootAddress(), true, 28u);
        dialog.RethrowEventError();
        Require(selections.size() == before_enter + 1u && selections.back() == 2u,
                "right then Enter invokes actual No button through shared navigation and DLL mouse events");
        dialog.Show(U"Confirm?", 3u, selection, sound);
        dispatch(dialog.RootAddress(), true, 28u);
        dialog.RethrowEventError();
        Require(selections.back() == 1u, "initial navigation selection Enter invokes actual Yes button");
        std::weak_ptr<int> callback_lifetime;
        dialog.Show(U"Visible first prompt",3u,selection,sound);
        Require(parent() && modal(),"first shared prompt is modal");
        dialog.Show(U"Replace visible prompt",3u,selection,sound);
        Require(parent() && modal(),"replacing a visible shared prompt preserves modal input");
        {
            const auto token = std::make_shared<int>(7);
            callback_lifetime = token;
            dialog.Show(U"Owned callback", 3u, [token](std::uint32_t) {}, sound);
        }
        Require(!callback_lifetime.expired(), "prompt owns a copied callback after caller scope ends");
        dialog.Show(U"Information", 4u, {}, sound);
        Require(callback_lifetime.expired(), "replacing prompt releases its previous copied callback");
        dispatch(dialog.RootAddress(), true, 1u);
        dialog.RethrowEventError();
        Require(dialog.LastSelection() == 2u && !parent() && !modal(),
                "Escape without a callback preserves original default selection");
        dialog.Show(U"First prompt", 3u, [&](std::uint32_t value) {
            Require(value == 1u, "reentrant prompt receives original Yes selection");
            dialog.Show(U"Second prompt", 4u, selection, sound);
        }, sound);
        dispatch(dialog.Window("BtYes"), false);
        dialog.RethrowEventError();
        Require(parent() && modal() && visible("BtOk") && !visible("BtYes"),
                "cloned callback safely replaces its own prompt during dispatch");
        dispatch(dialog.Window("BtOk"), false);
        dialog.RethrowEventError();
        Require(selections.back() == 4u && !parent() && !modal(), "replacement prompt routes new callback and closes");
        dialog.Show(U"Throwing callback", 3u, [](std::uint32_t) { throw std::runtime_error("selection probe"); }, sound);
        dispatch(dialog.Window("BtYes"), false);
        caught = false;
        try { dialog.RethrowEventError(); } catch (const std::runtime_error&) { caught = true; }
        Require(caught && !parent() && !modal(), "throwing selection callback remains closed and is contained at DLL boundary");
        dialog.Show(U"Destructor prompt", 4u, selection, sound);
    }
    const auto system = Export<void* (__cdecl*)()>(module,
        "?getSingletonPtr@System@CEGUI@@SAPAV12@XZ")();
    Require(!Export<void* (__thiscall*)(const void*)>(module,
        "?getModalTarget@System@CEGUI@@QBEPAVWindow@2@XZ")(system),
        "destroying an open prompt releases the original DLL modal target");
    desktop.Check("common dialog construction preserves focus and cursor");
    std::cout << "Native CommonDialog shared construction, original controls, IDs, DLL events, re-create and exception boundary verified\n";
    std::cout << "Native CommonDialog text sizing, modal flags, sound policy, cloned callbacks, keyboard Escape/Enter and reentrant replacement verified\n";
}
