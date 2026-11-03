#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_widget_images.h"
#include "pal4/native_cegui_text_conversion.h"
#include "pal4/native_game_database.h"
#include "pal4/system_setting_stats.h"
#include "pal4/native_system_setting_stats_data.h"
#include "pal4/native_event_handler_array.h"
#include "pal4/native_dialog_settings_snapshot.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "../native/ui/cegui_exports_native.h"

#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <limits>
#include <vector>
#include <bit>
#include <algorithm>
#include <cmath>

void VerifyNativeNumberedListRender(HMODULE module, void* list,
    const pal4::NativeGameDatabase& database, const std::filesystem::path& game_root,
    pal4::NativeCeguiUiSession& ui);
void VerifyNativeSystemSettingNavigation(pal4::NativeCeguiUiSession& ui,
    const std::filesystem::path& game_root, const pal4::NativeGameDatabase& database);
void VerifyNativeCommonDialogLayout(pal4::NativeCeguiUiSession& ui,
    const std::filesystem::path& game_root);

namespace {
void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

// Deliberately scoped data probe: production callers must supply live services.
// Exercises native DLL rendering inputs independently of unfinished game stats.
class StatsDataProbe final : public pal4::ISystemSettingStatsDataRuntime {
public:
    std::array<std::uint32_t, 4> settings{0u, 2u, 0u, 1u};
    std::array<float, 2> audio{0.25f, 0.75f};
    std::array<std::int32_t, 7> entries{2, 3, 128, 256, 100000,
        std::numeric_limits<std::int32_t>::max(), std::numeric_limits<std::int32_t>::min()};
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto begin = GlobalData();
        if (address < begin || address - begin >= sizeof(settings) || (address - begin) % 4u)
            throw std::out_of_range("stat probe global field");
        return settings[(address - begin) / 4u];
    }
    float ReadFloat(std::uint32_t address) override {
        const auto begin = AudioManager();
        if (address < begin || address - begin >= sizeof(audio) || (address - begin) % 4u)
            throw std::out_of_range("stat probe audio field");
        return audio[(address - begin) / 4u];
    }
    std::uint32_t GlobalData() override { return reinterpret_cast<std::uint32_t>(settings.data()); }
    double CalculateStatValue(std::uint32_t global) override {
        Require(global == GlobalData(), "stat probe global identity");
        return 17.5;
    }
    float UsedSaveCount() override { return 3.0f; }
    std::int32_t AvailableSaveCount() override { return 12; }
    void InitializeInventory() override {}
    double InventoryRatioOne() override { return 0.125; }
    double InventoryRatioTwo() override { return 0.5; }
    double NormalizedStat(std::uint32_t global) override {
        Require(global == GlobalData(), "stat probe normalized identity");
        return 0.625;
    }
    std::uint32_t Game() override { return reinterpret_cast<std::uint32_t>(entries.data()); }
    std::int32_t GameEntry(std::uint32_t game, std::uint32_t index) override {
        Require(game == Game(), "stat probe game identity");
        return entries.at(index);
    }
    std::uint32_t AudioManager() override { return reinterpret_cast<std::uint32_t>(audio.data()); }
};
}

void VerifyNativeSystemSettingLayout(pal4::NativeCeguiUiSession& ui,
                                    const std::filesystem::path& game_root,
                                    const pal4::NativeGameDatabase& database) {
    using pal4::detail::Export;
    using pal4::detail::NativeString;
    StatsDataProbe data;
    Require(!ui.RefreshSystemSettingStats(database, data), "stat refresh rejects missing layout");
    Require(!ui.ResetSystemSettingPage(), "setting reset rejects missing layout");
    Require(!ui.InitializeSystemSettingSaveControls(database), "save controls reject missing layout");
    if (!ui.LoadOriginalSystemSettingLayout(game_root))
        std::cerr << ui.LastError() << '\n';
    const auto root = ui.SystemSettingRootAddress();
    Require(root && ui.LoadOriginalSystemSettingLayout(game_root) &&
                ui.SystemSettingRootAddress() == root,
            "original SystemSetting layout loads once with stable root identity");
    const auto module = GetModuleHandleA("CEGUIBase.dll");
    const auto manager = Export<void* (__cdecl*)()>(module,
        "?getSingletonPtr@WindowManager@CEGUI@@SAPAV12@XZ")();
    const auto get = Export<void* (__thiscall*)(const void*, const void*)>(module,
        "?getWindow@WindowManager@CEGUI@@QBEPAVWindow@2@ABVString@2@@Z");
    const auto find = [&](std::string_view suffix) {
        NativeString name(module, std::string("SystemSetting/") + std::string(suffix));
        return get(manager, name.Data());
    };
    const auto visible = Export<bool (__thiscall*)(const void*, bool)>(module,
        "?isVisible@Window@CEGUI@@QBE_N_N@Z");
    const auto set_visible = Export<void (__thiscall*)(void*, bool)>(module,
        "?setVisible@Window@CEGUI@@QAEX_N@Z");
    const auto set_selected = Export<void (__thiscall*)(void*, bool)>(module,
        "?setSelected@ButtonBase@CEGUI@@QAEX_N@Z");
    const auto selected = Export<bool (__thiscall*)(const void*)>(module,
        "?isSelected@ButtonBase@CEGUI@@QBE_NXZ");
    Require(!visible(reinterpret_cast<void*>(root), true), "system setting starts locally hidden");
    constexpr const char* groups[]{"SettingWindow0", "SettingWindow1", "SettingWindow2",
        "SaveWindow0", "SaveWindow1", "SaveWindow2", "Savewindowblanknew"};
    constexpr const char* buttons[]{"BtnSaveData", "BtnLoadData", "BtnSystemSetting", "BtnExit"};
    for (const auto* name : groups) set_visible(find(name), true);
    for (const auto* name : buttons) set_selected(find(name), true);
    Require(ui.ResetSystemSettingPage() && ui.ResetSystemSettingPage(),
            "shared system-page reset operates repeatedly on original DLL controls");
    for (const auto* name : groups)
        Require(!visible(find(name), true), "system-page reset hides both groups and blank panel");
    for (const auto* name : buttons)
        Require(!selected(find(name)), "system-page reset clears every main button selection");
    const auto image = Export<const void* (__thiscall*)(const void*)>(module,
        "?getImage@StaticImage@CEGUI@@QBEPBVImage@2@XZ")(find("cucunyouxi"));
    pal4::NativeCeguiWidgetImages images(module);
    Require(reinterpret_cast<std::uint32_t>(image) == images.Image("zi0", "xitong"),
            "native system title uses imageset zi0 and image xitong in DLL argument order");
    Require(!visible(reinterpret_cast<void*>(root), true), "page preparation preserves hidden root");
    std::cout << "Native SystemSetting CPK layout and shared system-page reset verified\n";

    // Exercise the real activity-array adapter with strict scoped dispatchers.
    // Persistent options/save input implementations remain a separate gate.
    const auto foreground = GetForegroundWindow();
    POINT cursor{};
    Require(GetCursorPos(&cursor), "options page cursor snapshot");
    std::vector<std::pair<std::uint32_t, std::uint32_t>> activity_calls;
    constexpr std::uint32_t previous = 0x11223344u;
    pal4::NativeEventHandlerArray activity(
        [&](std::uint32_t object, std::uint32_t slot) -> std::uint32_t {
            Require((object == previous || object == ui.SystemSettingOptionsInputAddress() ||
                     object == ui.SystemSettingSaveInputAddress()) &&
                    (slot == 12u || slot == 16u), "only scoped activity activate/resume callbacks");
            activity_calls.emplace_back(object, slot);
            return 0u;
        }, [](std::uint32_t, std::uint32_t, std::uint32_t) -> std::uint8_t {
            throw std::logic_error("page display must not dispatch keyboard input");
        });
    for (unsigned pass = 0; pass < 2u; ++pass) {
        (void)activity.Insert(previous);
        (void)activity.Insert(ui.SystemSettingSaveInputAddress());
        activity_calls.clear();
        for (const auto* name : groups) set_visible(find(name), false);
        Require(ui.ShowSystemSettingOptions(activity), "shared options-page display reaches DLL and activity array");
        for (unsigned i = 0; i < 7u; ++i)
            Require(visible(find(groups[i]), true) == (i < 3u || i == 6u),
                    "options groups and blank shown while save groups hidden");
        Require(activity.Size() == 2u && activity.GetLast() == ui.SystemSettingOptionsInputAddress() &&
                    activity_calls == std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                        {previous, 16u}, {ui.SystemSettingOptionsInputAddress(), 12u}},
                "save activity removal resumes previous owner before activating options member");
        const auto options = ui.SystemSettingOptionsInputAddress();
        Require(*reinterpret_cast<const std::uint32_t*>(options + 24u) ==
                    reinterpret_cast<std::uint32_t>(find("ScroolAudio1")) &&
                *reinterpret_cast<const std::uint32_t*>(options + 28u) == 0u,
                "options input begins at original audio slider with zero navigation index");
        Require(!visible(reinterpret_cast<void*>(root), true), "options preparation preserves hidden root");
        (void)activity.Remove(options);
        activity.ReleaseStorage();
        Require(ui.ResetSystemSettingPage(), "options page can return to reset state");
    }
    POINT cursor_after{};
    Require(GetCursorPos(&cursor_after) && foreground == GetForegroundWindow() &&
            cursor.x == cursor_after.x && cursor.y == cursor_after.y,
            "options page preserves foreground and cursor");
    std::cout << "Native SystemSetting options groups, slider and activity switch verified\n";

    auto* list = find("List");
    using Flag = void (__thiscall*)(void*, bool);
    const auto get_text_save = Export<const void* (__thiscall*)(const void*)>(module,
        "?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto c_str_save = Export<const char* (__thiscall*)(const void*)>(module,
        "?c_str@String@CEGUI@@QBEPBDXZ");
    for (unsigned pass = 0; pass < 2u; ++pass) {
        NativeString seed(module, "seed");
        Export<void (__thiscall*)(void*, const void*, std::uint32_t, float)>(module,
            "?addColumn@MultiColumnList@CEGUI@@QAEXABVString@2@IM@Z")(list, seed.Data(), 7u, 1.0f);
        (void)Export<std::uint32_t (__thiscall*)(void*, std::uint32_t)>(module,
            "?addRow@MultiColumnList@CEGUI@@QAEII@Z")(list, 0u);
        Export<void (__thiscall*)(void*, std::uint32_t)>(module,
            "?setSortDirection@MultiColumnList@CEGUI@@QAEXW4SortDirection@ListHeaderSegment@2@@Z")(list, 1u);
        constexpr const char* set_flags[]{
            "?setUserColumnSizingEnabled@MultiColumnList@CEGUI@@QAEX_N@Z",
            "?setUserColumnDraggingEnabled@MultiColumnList@CEGUI@@QAEX_N@Z",
            "?setUserSortControlEnabled@MultiColumnList@CEGUI@@QAEX_N@Z"};
        for (auto* name : set_flags) Export<Flag>(module, name)(list, true);
        Export<Flag>(module, "?setAlwaysOnTop@Window@CEGUI@@QAEX_N@Z")(find("ImgDataMask"), false);
        Export<Flag>(module, "?setEnabled@Window@CEGUI@@QAEX_N@Z")(find("ImgDataMask"), true);
        if (!ui.InitializeSystemSettingSaveControls(database)) std::cerr << ui.LastError() << '\n';
        Require(ui.LastError().empty(), "native save-control initialization succeeds");
        for (std::uint32_t i = 0; i < 10u; ++i) {
            auto* button = find("BtnData" + std::to_string(i));
            const auto source = database.FindTableTextById("GameString", 205u + i);
            const auto utf8 = source ? pal4::ConvertGameTextToUtf8(*source) : std::nullopt;
            Require(utf8 && *utf8 == c_str_save(get_text_save(button)) &&
                Export<std::uint32_t (__thiscall*)(const void*)>(module,
                    "?getID@Window@CEGUI@@QBEIXZ")(button) == i,
                "all ten page labels and IDs match original GameString205..214");
        }
        Require(std::string(c_str_save(get_text_save(find("StaticDataMask")))) ==
                    c_str_save(get_text_save(find("BtnData0"))) &&
                Export<std::uint32_t (__thiscall*)(const void*)>(module,
                    "?getHorizontalFormatting@StaticText@CEGUI@@QBE?AW4HorzFormatting@12@XZ")(
                        find("StaticDataMask")) == 2u,
                "save page label copies first page and uses original horizontal formatting");
        Require(Export<bool (__thiscall*)(const void*)>(module,
                    "?isAlwaysOnTop@Window@CEGUI@@QBE_NXZ")(find("ImgDataMask")) &&
                Export<bool (__thiscall*)(const void*, bool)>(module,
                    "?isDisabled@Window@CEGUI@@QBE_N_N@Z")(find("ImgDataMask"), true),
                "save mask stays above controls and cannot receive input");
        constexpr const char* counts[]{"?getRowCount@MultiColumnList@CEGUI@@QBEIXZ",
            "?getColumnCount@MultiColumnList@CEGUI@@QBEIXZ",
            "?getSortDirection@MultiColumnList@CEGUI@@QBE?AW4SortDirection@ListHeaderSegment@2@XZ"};
        for (auto* name : counts) Require(Export<std::uint32_t (__thiscall*)(const void*)>(module, name)(list) == 0u,
            "save initialization clears rows, columns and sorting");
        constexpr const char* get_flags[]{
            "?isUserColumnSizingEnabled@MultiColumnList@CEGUI@@QBE_NXZ",
            "?isUserColumnDraggingEnabled@MultiColumnList@CEGUI@@QBE_NXZ",
            "?isUserSortControlEnabled@MultiColumnList@CEGUI@@QBE_NXZ"};
        for (auto* name : get_flags) Require(!Export<bool (__thiscall*)(const void*)>(module, name)(list),
            "save list forbids user sizing, dragging and sorting");
    }
    pal4::NativeGameDatabase missing_page_database;
    const std::string page_before(c_str_save(get_text_save(find("BtnData0"))));
    Require(!ui.InitializeSystemSettingSaveControls(missing_page_database) &&
            !ui.LastError().empty() && page_before == c_str_save(get_text_save(find("BtnData0"))) &&
            ui.InitializeSystemSettingSaveControls(database),
            "missing page text reports failure before changing controls and valid data recovers");
    Require(!visible(reinterpret_cast<void*>(root), true), "save-control initialization preserves hidden root");
    std::cout << "Native SystemSetting save page labels, IDs, mask and list controls verified\n";
    VerifyNativeNumberedListRender(module, list, database, game_root, ui);

    pal4::DialogManagerState page_globals;
    page_globals.SetWord(44u, 23u);
    page_globals.SetWord(48u, std::bit_cast<std::uint32_t>(25.0f));
    const auto grid_item = Export<void* (__thiscall*)(const void*, const void*)>(module,
        "?getItemAtGridReference@MultiColumnList@CEGUI@@QBEPAVListboxItem@2@ABUMCLGridRef@2@@Z");
    const auto row_count = Export<std::uint32_t (__thiscall*)(const void*)>(module,
        "?getRowCount@MultiColumnList@CEGUI@@QBEIXZ");
    const auto hover_row = Export<std::int32_t (__thiscall*)(void*)>(module,
        "?getHoverRowIndex@MultiColumnList@CEGUI@@QAEHXZ");
    pal4::NativeGameSaveSummaryReader page_reader;
    for (const auto page_index : {0, 9, 0}) {
        if (!ui.PopulateSystemSettingSavePage(game_root, database, page_globals, page_index, true))
            std::cerr << ui.LastError() << '\n';
        Require(ui.LastError().empty() && row_count(list) == 10u && hover_row(list) == 3,
                "shared save-page fill creates ten native game rows and restores slot remainder");
        const auto scrollbar = *reinterpret_cast<void* const*>(reinterpret_cast<std::uintptr_t>(list) + 1204u);
        Require(Export<float (__thiscall*)(const void*)>(module,
                    "?getScrollPosition@Scrollbar@CEGUI@@QBEMXZ")(scrollbar) == 25.0f,
                "native load page restores saved scrollbar position");
        for (std::uint32_t row = 0u; row < 10u; ++row) {
            const std::array<std::uint32_t, 2> grid{row, 0u};
            const auto address = reinterpret_cast<std::uint32_t>(grid_item(list, grid.data()));
            const auto slot = page_index * 10 + row;
            const auto summary = page_reader.ReadSlot(game_root, slot);
            const auto word = [&](unsigned offset) {
                return *reinterpret_cast<const std::uint32_t*>(address + offset);
            };
            Require(address && word(440u) == slot && word(492u) == (summary ? summary->header.version : 0u),
                    "every native row preserves actual slot and loaded-or-empty summary state");
            const auto table = *reinterpret_cast<const std::uint32_t* const*>(address);
            std::array<float, 2> item_size{};
            reinterpret_cast<void* (__thiscall*)(void*, void*)>(table[1])(
                reinterpret_cast<void*>(address), item_size.data());
            Require(item_size[1] > 0.0f && item_size[1] ==
                    Export<float (__thiscall*)(const void*)>(module,
                        "?getPageSize@Scrollbar@CEGUI@@QBEMXZ")(scrollbar) * 0.25f,
                    "native row height matches quarter of original DLL scrollbar page size");
            if (summary) Require(word(508u) == pal4::NativeCeguiWidgetImages(module).Image(
                    "GameSaveImage" + std::to_string(slot), "full_image"),
                    "loaded page row borrows actual slot thumbnail");
        }
        auto* page_mask = find("ImgDataMask");
        Require(Export<void* (__thiscall*)(const void*)>(module,
                    "?getParent@Window@CEGUI@@QBEPAV12@XZ")(page_mask) == find("BtnData" + std::to_string(page_index)) &&
                Export<std::uint32_t (__thiscall*)(const void*)>(module,
                    "?getID@Window@CEGUI@@QBEIXZ")(page_mask) == page_index &&
                !Export<bool (__thiscall*)(const void*)>(module,
                    "?isClippedByParent@Window@CEGUI@@QBE_NXZ")(page_mask),
                "shared page fill reparents unclipped mask onto selected original page button");
        Require(std::string(c_str_save(get_text_save(find("StaticDataMask")))) ==
                c_str_save(get_text_save(find("BtnData" + std::to_string(page_index)))),
                "shared page fill copies selected original page title");
    }
    const std::array<std::uint32_t, 2> first_grid{};
    auto* first_before_invalid = grid_item(list, first_grid.data());
    Require(!ui.PopulateSystemSettingSavePage(game_root, database, page_globals, -1, true) &&
            !ui.PopulateSystemSettingSavePage(game_root, database, page_globals, 10, true) &&
            grid_item(list, first_grid.data()) == first_before_invalid,
            "invalid page range preserves previous list without indexed frame access");
    Require(!ui.PopulateSystemSettingSavePage(game_root, missing_page_database, page_globals, 0, true) &&
            grid_item(list, first_grid.data()) == first_before_invalid,
            "missing database fails before releasing previous native page");
    Require(ui.PopulateSystemSettingSavePage(game_root, database, page_globals, 0, false) &&
            row_count(list) == 10u && hover_row(list) == 0,
            "save mode selects first row instead of restoring load-mode hover");
    Require(!visible(reinterpret_cast<void*>(root), true), "page fill preserves hidden frame staging");
    std::cout << "Native SystemSetting full ten-row save/load pages, summaries, thumbnails, mask, repeat reset and recovery verified\n";
    VerifyNativeSystemSettingNavigation(ui, game_root, database);
    VerifyNativeCommonDialogLayout(ui, game_root);

    if (!ui.RefreshSystemSettingStats(database, data)) std::cerr << ui.LastError() << '\n';
    Require(ui.LastError().empty(), "complete statistic refresh reaches native DLL controls");
    const auto get_text = Export<const void* (__thiscall*)(const void*)>(module,
        "?getText@Window@CEGUI@@QBEABVString@2@XZ");
    const auto c_str = Export<const char* (__thiscall*)(const void*)>(module,
        "?c_str@String@CEGUI@@QBEPBDXZ");
    constexpr double expected_values[]{17.5, 25.0, 12.5, 50.0, 62.5, 40.0, 60.0};
    for (unsigned i = 0; i < 12u; ++i) {
        const auto format = database.FindTableTextById("GameString", 156u + i);
        Require(format.has_value(), "all statistic GameString formats exist in original database");
        const std::string format_text(*format);
        const auto size = i < 7u
            ? std::snprintf(nullptr, 0u, format_text.c_str(), expected_values[i])
            : std::snprintf(nullptr, 0u, format_text.c_str(), data.entries[i - 5u]);
        Require(size >= 0, "expected original statistic formatting");
        std::vector<char> expected(static_cast<std::size_t>(size) + 1u);
        if (i < 7u) std::snprintf(expected.data(), expected.size(), format_text.c_str(), expected_values[i]);
        else std::snprintf(expected.data(), expected.size(), format_text.c_str(), data.entries[i - 5u]);
        const auto utf8 = pal4::ConvertGameTextToUtf8({expected.data(), static_cast<std::size_t>(size)});
        if (!utf8 || *utf8 != c_str(get_text(find("StaticInfo" + std::to_string(i + 1u)))))
            std::cerr << "Statistic text mismatch index=" << i << " format=" << format_text
                      << " expected=" << (utf8 ? *utf8 : "conversion failed")
                      << " actual=" << c_str(get_text(find("StaticInfo" + std::to_string(i + 1u)))) << '\n';
        Require(utf8 && *utf8 == c_str(get_text(find("StaticInfo" + std::to_string(i + 1u)))),
                "native text equals original GameString formatting and ACP conversion");
    }
    const auto slider = Export<float (__thiscall*)(const void*)>(module,
        "?getCurrentValue@Slider@CEGUI@@QBEMXZ");
    const auto maximum = Export<float (__thiscall*)(const void*)>(module,
        "?getMaxValue@Slider@CEGUI@@QBEMXZ");
    Require(slider(find("ScroolAudio1")) == 0.75f && slider(find("ScroolAudio2")) == 0.25f &&
                maximum(find("ScroolAudio1")) == 1.0f && maximum(find("ScroolAudio2")) == 1.0f,
            "original sliders use full range and inverse audio levels");
    constexpr const char* choices[]{"BtnSeq1", "BtnSeq2", "BtnSeq3", "BtnClip1", "BtnClip2",
        "BtnAuto1", "BtnAuto2", "BtnAuto3", "BtnIntro1", "BtnIntro2"};
    for (unsigned i = 0; i < 10u; ++i)
        Require(selected(find(choices[i])) == (i == 2u || i == 4u || i == 6u || i == 9u),
                "real option buttons reflect all four global setting groups");
    data.settings = {2u, 99u, 1u, 0u};
    data.audio = {1.0f, 0.0f};
    Require(ui.RefreshSystemSettingStats(database, data), "repeat refresh rereads changed data");
    for (unsigned i = 0; i < 10u; ++i)
        Require(selected(find(choices[i])) == (i == 3u || i == 7u || i == 8u),
                "repeat refresh clears stale selections and handles unsupported difficulty");
    Require(slider(find("ScroolAudio1")) == 0.0f && slider(find("ScroolAudio2")) == 1.0f,
            "repeat refresh rereads both audio endpoints");
    pal4::NativeGameDatabase empty_database;
    Require(!ui.RefreshSystemSettingStats(empty_database, data) && !ui.LastError().empty() &&
                ui.RefreshSystemSettingStats(database, data),
            "missing GameString reports failure and subsequent valid refresh recovers");
    Require(!visible(reinterpret_cast<void*>(root), true), "stat refresh preserves hidden root");
    std::cout << "Native SystemSetting twelve texts, ten options and two audio sliders verified\n";
}

void VerifyNativeSystemSettingLiveData(pal4::NativeCeguiUiSession& ui,
    const pal4::NativeGameDatabase& database, pal4::NativeDialogSettingsSnapshot& settings,
    pal4::NativePalGameIVTextureStorage& game, pal4::NativeSystemSettingStatsData& data) {
    const auto saved_state = settings.State();
    const auto first = settings.State().Word(184u), end = settings.State().Word(188u);
    std::vector<std::uint32_t> saved_stories;
    for (auto address = first; address < end; address += 4u)
        saved_stories.push_back(settings.StoryWord(address));
    const auto source_address = game.GlobalAddress(0x90dec0u);
    auto& source = *reinterpret_cast<std::uint32_t*>(source_address);
    const auto saved_source = source;
    const std::array saved_audio{settings.AudioLevel(0u), settings.AudioLevel(1u)};
    const auto foreground = GetForegroundWindow();
    POINT cursor{};
    Require(GetCursorPos(&cursor) != FALSE, "live statistic cursor snapshot");

    const auto& monster = database.Tables().at("Monster");
    std::size_t count_field = monster.fields.size();
    for (std::size_t i = 0; i < monster.fields.size(); ++i)
        if (monster.fields[i].name == "COUNT") count_field = i;
    Require(count_field < monster.fields.size(), "original Monster COUNT schema");
    std::int32_t expected_count = 0;
    for (const auto& row : monster.rows)
        if (std::bit_cast<std::int32_t>(std::get<std::uint32_t>(row.at(count_field))) > 0)
            ++expected_count;
    Require(expected_count > 0 && data.AvailableSaveCount() == expected_count,
            "shared availability counts original Monster COUNT entries");
    settings.State().SetWord(144u, 3u);
    settings.State().SetWord(176u, 5u);
    settings.State().SetWord(160u, 9u);
    settings.State().SetWord(128u, 7u);
    Require(data.UsedSaveCount() == 3.0f &&
                data.InventoryRatioOne() == 5.0 / database.Tables().at("Equip").rows.size() &&
                data.InventoryRatioTwo() == 9.0 / database.Tables().at("Property").rows.size() &&
                data.NormalizedStat(data.GlobalData()) == 7.0 * std::bit_cast<float>(0x3c053408u),
            "shared ratios and normalization read live global counters and original table counts");
    source = 9u;
    settings.WriteStoryWord(first, 3u);
    const auto score = data.CalculateStatValue(data.GlobalData());
    Require(settings.StoryWord(first) == 9u && std::isfinite(score),
            "live Story source merges its maximum into loaded settings before scoring");
    // Independent table-row reference for the current merged Story state.
    const auto& mission = database.Tables().at("Mission");
    std::size_t trunk = mission.fields.size(), quest = trunk, percent = trunk;
    for (std::size_t i = 0; i < mission.fields.size(); ++i) {
        if (mission.fields[i].name == "TRUNK") trunk = i;
        if (mission.fields[i].name == "QUEST_ID") quest = i;
        if (mission.fields[i].name == "STORY_PER") percent = i;
    }
    std::vector<const std::vector<pal4::NativeGameDbValue>*> selected(saved_stories.size());
    for (const auto& row : mission.rows) {
        const auto slot = std::bit_cast<std::int32_t>(std::get<std::uint32_t>(row.at(trunk)));
        if (slot < 0 || static_cast<std::size_t>(slot) >= selected.size()) continue;
        const auto threshold = std::bit_cast<std::int32_t>(std::get<std::uint32_t>(row.at(quest)));
        if (threshold > std::bit_cast<std::int32_t>(settings.StoryWord(first + 4u * slot))) continue;
        if (!selected[slot] || threshold > std::bit_cast<std::int32_t>(
                std::get<std::uint32_t>(selected[slot]->at(quest)))) selected[slot] = &row;
    }
    double expected_score = 0.0;
    for (const auto* row : selected) if (row) expected_score += std::get<float>(row->at(percent));
    Require(score == static_cast<float>(expected_score),
            "shared Mission sort and scoring match original database row reference");
    settings.State().SetWord(0x3cu, 4096u);
    Require(data.GameEntry(data.Game(), 2u) == 4096,
            "live GameData getter preserves full-width global statistic");
    settings.SetAudioLevel(0u, 0.25f);
    settings.SetAudioLevel(1u, 0.75f);
    if (!ui.RefreshSystemSettingStats(database, data)) std::cerr << ui.LastError() << '\n';
    Require(ui.LastError().empty(), "native statistic controls refresh from actual game/settings/database owners");
    Require(data.ReadFloat(data.AudioManager()) == 0.25f &&
                data.ReadFloat(data.AudioManager() + 4u) == 0.75f,
            "statistic audio reads current settings levels");
    settings.SetAudioLevel(0u, 1.0f);
    Require(data.ReadFloat(data.AudioManager()) == 1.0f && ui.RefreshSystemSettingStats(database, data),
            "live data adapter observes later changes without reconstruction");

    source = saved_source;
    settings.State() = saved_state;
    for (std::size_t i = 0; i < saved_stories.size(); ++i)
        settings.WriteStoryWord(first + static_cast<std::uint32_t>(i) * 4u, saved_stories[i]);
    for (unsigned i = 0; i < 2u; ++i) settings.SetAudioLevel(i, saved_audio[i]);
    Require(ui.RefreshSystemSettingStats(database, data), "refresh restores original runtime data after probe");
    POINT after{};
    Require(GetForegroundWindow() == foreground && GetCursorPos(&after) &&
                cursor.x == after.x && cursor.y == after.y,
            "native live statistic refresh preserves foreground and pointer position");
    std::cout << "Native SystemSetting live GameDB, global counters, Story maxima and audio binding verified\n";
}
