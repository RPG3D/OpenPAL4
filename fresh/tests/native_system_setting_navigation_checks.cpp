#include "support/native_desktop_observation.h"
#include "pal4/native_system_setting_save_navigation.h"
#include "pal4/native_system_setting_save_confirmation.h"
#include "pal4/native_system_setting_save_result.h"
#include "pal4/native_dialog_settings_snapshot.h"
#include "pal4/dialog_manager_settings.h"
#include "pal4/native_game_database.h"
#include "pal4/native_cegui_text_conversion.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/dialog_manager_lifecycle.h"
#include "../native/ui/cegui_exports_native.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}
}

void VerifyNativeSystemSettingNavigation(pal4::NativeCeguiUiSession& ui,
    const std::filesystem::path& game_root, const pal4::NativeGameDatabase& database) {
    using pal4::detail::Export;
    using pal4::detail::NativeString;
    const auto module = GetModuleHandleA("CEGUIBase.dll");
    const auto manager = Export<void* (__cdecl*)()>(module,
        "?getSingletonPtr@WindowManager@CEGUI@@SAPAV12@XZ")();
    const auto find = [&](std::string_view suffix) {
        NativeString name(module, "SystemSetting/" + std::string(suffix));
        return Export<void* (__thiscall*)(const void*, const void*)>(module,
            "?getWindow@WindowManager@CEGUI@@QBEPAVWindow@2@ABVString@2@@Z")(manager, name.Data());
    };
    auto* list = find("List");
    auto* mask = find("ImgDataMask");
    const auto page_id = [&] { return Export<std::uint32_t (__thiscall*)(const void*)>(module,
        "?getID@Window@CEGUI@@QBEIXZ")(mask); };
    const auto hover = [&] { return Export<std::int32_t (__thiscall*)(void*)>(module,
        "?getHoverRowIndex@MultiColumnList@CEGUI@@QAEHXZ")(list); };
    const auto clicked = Export<const void*>(module, "?EventClicked@PushButton@CEGUI@@2VString@2@B");
    const auto fire = Export<void (__thiscall*)(void*, const void*, void*, const void*)>(module,
        "?fireEvent@EventSet@CEGUI@@UAEXABVString@2@AAVEventArgs@2@0@Z");
    NativeString empty(module, "");
    const auto fire_click = [&](void* button, bool list_event = false) {
        alignas(4) std::array<std::byte, 12> event{};
        Export<void (__thiscall*)(void*, void*)>(module,
            "??0WindowEventArgs@CEGUI@@QAE@PAVWindow@1@@Z")(event.data(), button);
        fire(button, list_event ? Export<const void*>(module, "?EventMouseClick@Window@CEGUI@@2VString@2@B") : clicked, event.data(), empty.Data());
        Export<void (__thiscall*)(void*)>(module, "??1WindowEventArgs@CEGUI@@UAE@XZ")(event.data());
        ui.RethrowSystemSettingPageClickError();
    };
    const pal4::test::NativeDesktopObservation desktop;
    pal4::NativeDialogSettingsSnapshot settings;
    auto& globals = settings.State();
    const auto settings_root = std::filesystem::path("H:/PAL4/objs/save-result-settings") /
        std::to_string(GetCurrentProcessId());
    std::filesystem::create_directories(settings_root);
    pal4::NativeDialogSettingsFileWriter settings_writer(settings_root, settings);
    globals.SetWord(44u, 23u);
    globals.SetWord(48u, std::bit_cast<std::uint32_t>(0.0f));
    pal4::NativeSystemSettingSaveNavigation* navigation = nullptr;
    constexpr std::uint32_t previous = 0x11223344u;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> lifecycle;
    pal4::NativeEventHandlerArray active(
        [&](std::uint32_t member, std::uint32_t slot) -> std::uint32_t {
            lifecycle.emplace_back(member, slot);
            if (member == previous) return 1u;
            Require(navigation != nullptr, "save activity has a persistent native owner");
            return navigation->InvokeVirtual(member, slot);
        }, [&](std::uint32_t member, std::uint32_t slot, std::uint32_t event) -> std::uint8_t {
            Require(navigation != nullptr, "save key input has a persistent native owner");
            return navigation->InvokePredicate(member, slot, event);
        });
    (void)active.Insert(previous);
    unsigned confirmation_calls = 0u;
    pal4::NativeCommonDialogLayout dialog(module, ui, game_root);
    std::vector<std::uint32_t> save_results, load_results;
    std::vector<std::string> sounds;
    const auto modal = [&] { return Export<bool (__thiscall*)(const void*)>(module,
        "?getModalState@Window@CEGUI@@QBE_NXZ")(reinterpret_cast<void*>(dialog.RootAddress())); };
    const auto record = [&](std::vector<std::uint32_t>& results, std::uint32_t result) {
        Require(!modal() && !Export<void* (__thiscall*)(const void*)>(module,
            "?getParent@Window@CEGUI@@QBEPAV12@XZ")(reinterpret_cast<void*>(dialog.RootAddress())),
            "save confirmation closes before game action callback");
        results.push_back(result);
    };
    std::vector<std::string> game_calls;
    bool fail_save = false, fail_load = false;
    const auto verify_settings = [&] {
        pal4::NativeDialogSettingsSnapshot disk;
        pal4::NativeDialogSettingsFileReader reader(settings_root, disk);
        pal4::DialogManagerSettings::Load(disk.State(), reader);
        Require(disk.State().Word(44u) == globals.Word(44u) && disk.State().Word(48u) == globals.Word(48u),
                "selected slot and scrollbar persisted before game action");
    };
    pal4::NativeSystemSettingSaveResult result(module, ui, game_root, database, globals, settings_writer,
        [&](std::int32_t slot) {
            verify_settings();
            Require(!modal() && slot == static_cast<std::int32_t>(globals.Word(44u)), "save receives current selected slot after close");
            game_calls.emplace_back("save");
            if (fail_save) throw std::runtime_error("intentional save failure");
        }, [&](std::uint32_t event, std::uint32_t first, std::uint32_t second) {
            verify_settings();
            Require(!modal() && event == 0x8a80b4u && first == 0u && second == 0u, "load sends original close event after settings");
            game_calls.emplace_back("event");
        }, [&](const pal4::BinaryXmlNode& node) {
            Require(game_calls.back() == "event" && node.attributes.size() == 2u &&
                    node.FindAttribute("PALGAMEIV_LOADING_TYPE")->ToInt(99u) == 1u &&
                    node.FindAttribute("PALGAMEIV_LOADING_ARCHIVE")->ToInt(99u) == globals.Word(44u),
                    "load enters with original archive config after UI event");
            game_calls.emplace_back("load");
            if (fail_load) throw std::runtime_error("intentional loading failure");
        });
    pal4::NativeSystemSettingSaveConfirmation confirmation(module, ui, database, dialog,
        [&](std::string_view file, std::uint32_t repeat) {
            Require(file == "wa03.wav" && repeat == 1u, "save prompt requests original muted UI sound");
            sounds.emplace_back(file);
        }, [&](std::uint32_t value) { record(save_results, value); (void)result.Save(value); },
        [&](std::uint32_t value) { record(load_results, value); (void)result.Load(value); });
    const auto set_hover = [&](int row) {
        Export<void (__thiscall*)(void*, int)>(module,
            "?setHoverRowIndex@MultiColumnList@CEGUI@@QAEXH@Z")(list, row);
    };
    const auto check_text = [&](std::uint32_t id) {
        const auto source = database.FindTableTextById("GameString", id);
        Require(source.has_value(), "original save prompt GameString exists");
        const auto expected = pal4::ConvertGameTextToUtf8(*source);
        const auto text = Export<const void* (__thiscall*)(const void*)>(module,
            "?getText@Window@CEGUI@@QBEABVString@2@XZ")(reinterpret_cast<void*>(dialog.Window("Message")));
        Require(expected && *expected == Export<const char* (__thiscall*)(const void*)>(module,
            "?c_str@String@CEGUI@@QBEPBDXZ")(text), "native save prompt shows actual GameString text");
    };
    const auto choose = [&](const char* name) {
        fire_click(reinterpret_cast<void*>(dialog.Window(name)));
        dialog.RethrowEventError();
    };
    const auto key = [&](std::uint32_t code) {
        // Original key event layout: source Window+8, scan code+16.
        std::array<std::uint32_t, 5> event{};
        event[2] = ui.SystemSettingRootAddress();
        event[4] = code;
        return active.DispatchKeyDown(active.GetLast(), reinterpret_cast<std::uint32_t>(event.data()));
    };
    unsigned loaded_page9 = 0u;
    pal4::NativeGameSaveSummaryReader reader;
    for (int slot = 90; slot < 100; ++slot) loaded_page9 += reader.ReadSlot(game_root, slot).has_value();
    for (unsigned pass = 0; pass < 2u; ++pass) {
        pal4::NativeSystemSettingSaveNavigation owner(module, ui, game_root, database, globals, active,
            [&] { ++confirmation_calls; return confirmation.Show(); }); // Result probes never write original save files.
        navigation = &owner;
        lifecycle.clear();
        Require(ui.ShowSystemSettingData(game_root, database, globals, active, true),
                "original data-mode algorithm populates and activates the native page");
        Require(active.GetLast() == owner.InputAddress() && lifecycle ==
                std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                    {previous, 16u}, {owner.InputAddress(), 12u}},
                "native save input activates through original shared activity stack order");
        Require(page_id() == 2u && hover() == 3,
                "data mode selects the persisted slot page and row");
        const auto visible = Export<bool (__thiscall*)(const void*, bool)>(module,
            "?isVisible@Window@CEGUI@@QBE_N_N@Z");
        for (unsigned index = 0; index < 3u; ++index) {
            Require(!visible(find("SettingWindow" + std::to_string(index)), true) &&
                    visible(find("SaveWindow" + std::to_string(index)), true),
                    "data mode hides options groups and shows save groups");
        }
        Require(visible(find("Savewindowblanknew"), true) &&
                !visible(reinterpret_cast<void*>(ui.SystemSettingRootAddress()), true),
                "data panel is ready while root visibility remains with frame owner");
        globals.SetWord(44u, 100u);
        const auto invalid_textures = ui.TextureLoads();
        Require(!ui.ShowSystemSettingData(game_root, database, globals, active, true) &&
                page_id() == 2u && ui.TextureLoads() == invalid_textures && active.GetLast() == owner.InputAddress(),
                "invalid persisted slot preserves current page and input owner");
        globals.SetWord(44u, 23u);
        fire_click(find("BtnData0"));
        Require(key(200u) && hover() == 2 && key(208u) && hover() == 3,
                "persistent save key handler moves actual DLL row hover up and down");
        for (unsigned i = 0; i < 12u; ++i) Require(key(200u), "upper row boundary consumed");
        Require(hover() == 0, "native up navigation stops at first row");
        for (unsigned i = 0; i < 12u; ++i) Require(key(208u), "lower row boundary consumed");
        Require(hover() == 9, "native down navigation stops at last row");
        const auto textures_before_click = ui.TextureLoads();
        fire_click(find("BtnData9"));
        Require(page_id() == 9u && ui.TextureLoads() == textures_before_click + loaded_page9,
                "original DLL page event enters shared handler exactly once after rebind");
        const auto textures_before_boundary = ui.TextureLoads();
        Require(key(205u) && page_id() == 9u && ui.TextureLoads() == textures_before_boundary,
                "right boundary consumes key without rebuilding page");
        Require(key(203u) && page_id() == 8u && key(205u) && page_id() == 9u,
                "left and right keys repopulate actual previous and next pages");
        fire_click(find("BtnData0"));
        const auto before_left_boundary = ui.TextureLoads();
        Require(key(203u) && page_id() == 0u && ui.TextureLoads() == before_left_boundary,
                "left boundary consumes key without rebuilding first page");
        set_hover(0);
        Require(reader.ReadSlot(game_root, 0).has_value(), "load confirmation fixture uses real populated slot");
        const auto before_confirmation = confirmation_calls;
        Require(key(28u) && confirmation_calls == before_confirmation + 1u && modal(),
                "Enter delegates original confirmation action once");
        check_text(229u);
        const auto before_load = load_results.size();
        const auto calls_before_no = game_calls.size();
        choose("BtNo");
        Require(game_calls.size() == calls_before_no, "load No invokes no game services");
        Require(load_results.size() == before_load + 1u && load_results.back() == 2u,
                "load No survives source callback release");
        fire_click(list, true);
        Require(modal() && confirmation_calls == before_confirmation + 2u,
                "actual list MouseClick enters confirmation once across rebinding");
        choose("BtYes");
        Require(load_results.size() == before_load + 2u && load_results.back() == 1u,
                "load Yes reaches copied game action");
        Require(game_calls.size() == calls_before_no + 2u && game_calls[game_calls.size() - 2u] == "event" &&
                game_calls.back() == "load" && globals.Word(44u) == 0u,
                "confirmed load persists current selection and orders UI event before loading");
        globals.SetWord(44u, 23u);
        Require(ui.ShowSystemSettingData(game_root, database, globals, active, false) &&
                page_id() == 2u && hover() == 0,
                "save data mode selects persisted page but starts at its first row");
        fire_click(find("BtnData0"));
        set_hover(0);
        Require(key(28u) && modal(), "populated save slot opens overwrite prompt");
        check_text(109u);
        const auto before_save = save_results.size();
        const auto save_textures = ui.TextureLoads();
        choose("BtYes");
        Require(game_calls.back() == "save" && globals.Word(44u) == 0u &&
                ui.TextureLoads() > save_textures && page_id() == 0u,
                "confirmed save invokes writer then refreshes native page");
        Require(save_results.size() == before_save + 1u && save_results.back() == 1u,
                "overwrite Yes uses save action rather than load action");
        int empty_slot = -1;
        for (int slot = 0; slot < 100; ++slot)
            if (!reader.ReadSlot(game_root, slot)) { empty_slot = slot; break; }
        Require(empty_slot >= 0, "empty save fixture exists");
        fire_click(find("BtnData" + std::to_string(empty_slot / 10)));
        set_hover(empty_slot % 10);
        Require(key(28u) && modal(), "empty save slot opens new-save prompt");
        check_text(231u);
        choose("BtNo");
        Require(save_results.back() == 2u && save_results.size() == before_save + 2u,
                "new-save No reaches save selection callback");
        Require(ui.ShowSystemSettingData(game_root, database, globals, active, true), "restore load mode");
        fire_click(find("BtnData" + std::to_string(empty_slot / 10)));
        set_hover(empty_slot % 10);
        const auto before_empty = sounds.size();
        Require(key(28u) && !modal() && sounds.size() == before_empty,
                "empty load slot creates no dialog or sound");
        fire_click(find("BtnData0"));
        set_hover(0);
        const auto prior_load_calls = game_calls.size();
        fail_load = true;
        (void)confirmation.Show();
        bool load_error = false;
        try { choose("BtYes"); } catch (const std::runtime_error&) { load_error = true; }
        fail_load = false;
        Require(load_error && !modal() && game_calls.size() == prior_load_calls + 2u,
                "loading failure propagates through DLL event boundary after modal close");
        Require(ui.PopulateSystemSettingSavePage(game_root, database, globals, 0, false), "save failure fixture");
        set_hover(0);
        fail_save = true;
        (void)confirmation.Show();
        const auto before_failed_save = ui.TextureLoads();
        bool save_error = false;
        try { choose("BtYes"); } catch (const std::runtime_error&) { save_error = true; }
        fail_save = false;
        Require(save_error && !modal() && ui.TextureLoads() == before_failed_save,
                "failed save does not falsely refresh the page or swallow failure");
        globals.SetWord(44u, 23u);
        // This models returning to the main System page; no window is activated.
        lifecycle.clear();
        Require(key(1u) && active.GetLast() == previous && active.Size() == 1u &&
                lifecycle == std::vector<std::pair<std::uint32_t, std::uint32_t>>{{previous, 16u}},
                "Escape removes native save input and resumes previous activity before reset");
        for (const char* name : {"SaveWindow0", "SaveWindow1", "SaveWindow2"})
            Require(!Export<bool (__thiscall*)(const void*, bool)>(module,
                "?isVisible@Window@CEGUI@@QBE_N_N@Z")(find(name), true),
                "Escape uses actual system-page reset to hide save groups");
    }
    navigation = nullptr;
    const auto before_unbound = ui.TextureLoads();
    fire_click(find("BtnData9"));
    Require(ui.TextureLoads() == before_unbound && page_id() == 0u,
            "DLL-owned subscriptions safely ignore clicks after native owner destruction");
    active.ReleaseStorage();
    std::cout << "Native SystemSetting result actions persist settings, gate selections, order load config/event and propagate game failures verified\n";
    desktop.Check("native save navigation");
    std::cout << "Native SystemSetting persistent page events, keyboard rows/pages, Escape, Enter delegation and rebind verified\n";
    std::cout << "Native SystemSetting data groups, persisted page, load/save selection and activity verified\n";
}
