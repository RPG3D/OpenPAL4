#include "pal4/native_numbered_list_render.h"
#include "pal4/native_numbered_list_item.h"
#include "pal4/native_numbered_list_data.h"
#include "pal4/native_game_database.h"
#include "pal4/native_game_save_summary.h"
#include "pal4/native_scalar_text_format.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_widget_images.h"
#include "pal4/numbered_list_item.h"
#include "../native/ui/cegui_exports_native.h"
#include <bit>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <cstdlib>
#include <iostream>

namespace {
using pal4::detail::Export;
void Require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
std::uint32_t Address(const void* pointer) { return reinterpret_cast<std::uint32_t>(pointer); }

// Scoped data fixture; production game services are deliberately still borrowed.
struct Data final : pal4::INumberedListRenderDataRuntime {
    pal4::NativeCeguiWidgetImages images;
    unsigned icons = 0u, counts = 0u, records = 0u, players = 0u;
    explicit Data(HMODULE module) : images(module) {}
    std::uint32_t GetIconImage(std::uint32_t slot, std::uint32_t category, std::uint32_t variant) override {
        Require((slot == 0u || slot == 2u) && category == 6u && variant == 2u,
                "native numbered render retains sparse icon slots and variant");
        ++icons;
        return images.Image("gy0", "xuanzhekuang2");
    }
    std::string FormatItemCount(std::uint32_t trigger, std::uint32_t count) override {
        Require(trigger == 201u && (count == 3u || count == 7u), "numbered count formatting service");
        ++counts;
        return std::to_string(count);
    }
    std::vector<pal4::NumberedGameRecord> GetGameRecords() override {
        ++records;
        return {{2, "eligible", "scene"}, {99, "ineligible", "later"}};
    }
    bool TriggerObject(std::uint32_t id) override { Require(id == 11u, "numbered trigger ID"); return true; }
    void TriggerPlayer(std::uint32_t id) override { Require(id == 11u, "numbered player trigger ID"); ++players; }
};

struct NativeCache {
    // DLL constructor writes three16-byte vector objects through offset44.
    alignas(4) std::array<std::uint32_t, 12> words{};
    HMODULE module;
    explicit NativeCache(HMODULE value) : module(value) {
        Export<void* (__thiscall*)(void*)>(module, "??0RenderCache@CEGUI@@QAE@XZ")(words.data());
    }
    ~NativeCache() { Export<void (__thiscall*)(void*)>(module, "??1RenderCache@CEGUI@@QAE@XZ")(words.data()); }
    void Clear() { Export<void (__thiscall*)(void*)>(module, "?clearCachedImagery@RenderCache@CEGUI@@QAEXXZ")(words.data()); }
    unsigned Images() const { return (words[2] - words[1]) / 140u; }
    unsigned Texts() const { return (words[6] - words[5]) / 296u; }
};
}

void VerifyNativeNumberedListRender(HMODULE module, void* list,
    const pal4::NativeGameDatabase& database, const std::filesystem::path& game_root,
    pal4::NativeCeguiUiSession& ui) {
    Data data(module);
    pal4::NativeNumberedListRenderRuntime runtime(module, data);
    Require(runtime.GetSystemFont() != 0u, "numbered renderer borrows actual default font");
    alignas(4) std::array<std::byte, 520> item{};
    pal4::detail::NativeString number(module, "27");
    Export<void* (__thiscall*)(void*, const void*, std::uint32_t, void*, bool, bool)>(module,
        "??0ListboxItem@CEGUI@@QAE@ABVString@1@IPAX_N2@Z")(
            item.data(), number.Data(), 0u, nullptr, false, false);
    const auto address = Address(item.data());
    const auto write = [&](unsigned offset, std::uint32_t value) { runtime.WriteWord(address + offset, value); };
    write(496u, data.images.Image("kuang0", "kuang21"));
    write(500u, data.images.Image("gy0", "xuanzhekuang2"));
    write(504u, data.images.Image("kuang20", "kuang20"));
    const auto image = pal4::NumberedListItem::SetSize(address, std::bit_cast<std::uint32_t>(20.0f), runtime);
    std::array<std::uint32_t, 2> size{};
    Require(pal4::NumberedListItem::GetSize(address, Address(size.data()), runtime) == Address(size.data()) &&
        std::bit_cast<float>(size[0]) == runtime.ReadFloat(image + 28u) &&
        std::bit_cast<float>(size[1]) == 20.0f, "shared numbered sizes read actual DLL image width");
    const std::array<float, 4> rect{0.0f, 120.0f, 0.0f, 640.0f};
    const std::array<float, 4> clip{0.0f, 120.0f, 0.0f, 640.0f};
    NativeCache cache(module);
    pal4::NumberedListItem::Render(address, Address(cache.words.data()), rect, 0.75f, 0.5f, clip, runtime);
    Require(cache.Images() == 1u && cache.Texts() == 1u && !data.icons && !data.records,
            "empty save row caches native border and number without detailed game services");
    Require(runtime.ReadFloat(cache.words[1] + 20u) == 0.75f &&
            runtime.ReadFloat(cache.words[5] + 176u) == 0.75f &&
            runtime.ReadFloat(cache.words[5] + 180u) == 0.5f,
            "native cache preserves draw alpha and DLL modulated colour alpha separately");
    Require(runtime.IntersectRect({0.0f, 10.0f, 0.0f, 10.0f},
                {2.0f, 8.0f, 3.0f, 7.0f}) == std::array<float, 4>{2.0f, 8.0f, 3.0f, 7.0f},
            "native clipping uses original DLL rectangle intersection");
    cache.Clear();
    Require(!cache.Images() && !cache.Texts(), "original cache clears first row imagery and text");
    write(444u, 1u);
    write(448u, 1u);
    write(456u, 1u);
    write(464u, 3u);
    write(472u, 7u);
    write(480u, 5u);
    write(484u, 3661u);
    write(488u, 11u);
    write(492u, 1u);
    write(508u, data.images.Image("kuang0", "kuang21"));
    const std::string name = "Hero";
    write(428u, Address(name.c_str()));
    Export<void (__thiscall*)(void*, bool)>(module,
        "?setHovered@ListboxItem@CEGUI@@QAEX_N@Z")(item.data(), true);
    for (unsigned pass = 0; pass < 2u; ++pass) {
        pal4::NumberedListItem::Render(address, Address(cache.words.data()), rect, 1.0f, 0.5f, clip, runtime);
        Require(cache.Images() == 5u && cache.Texts() == 7u,
                "loaded hovered row caches thumbnail, detail, sparse icons, highlight and all texts");
        Require(data.icons == 2u * (pass + 1u) && data.counts == 2u * (pass + 1u) &&
                data.records == pass + 1u && data.players == pass + 1u,
                "complete native render rereads borrowed game services every draw");
        cache.Clear();
    }
    write(428u, 0u);
    Export<void (__thiscall*)(void*)>(module, "??1ListboxItem@CEGUI@@UAE@XZ")(item.data());
    std::cout << "Native NumberedListItem empty/detailed cache, alpha, clipping and sizes verified\n";

    const std::array<std::uint32_t, 13> empty{};
    pal4::NativeNumberedListItem owned(module, data, 26, empty, 0u, false, "", 20.0f);
    const auto native = owned.Address();
    const auto table = *reinterpret_cast<const std::uint32_t* const*>(native);
    std::array<std::uint32_t, 2> native_size{};
    Require(reinterpret_cast<void* (__thiscall*)(void*, void*)>(table[1])(
        reinterpret_cast<void*>(native), native_size.data()) == native_size.data() &&
        native_size == size, "native size virtual dispatch enters shared size method");
    // Original DLL base draw calls the host render virtual, with its real ABI.
    Export<void (__thiscall*)(void*, void*, const void*, float, float, const void*, std::uint32_t)>(module,
        "?draw@ListboxItem@CEGUI@@UBEXAAVRenderCache@2@ABVRect@2@MMPBV42@W4TextFormatting@2@@Z")(
            reinterpret_cast<void*>(native), cache.words.data(), rect.data(), 0.75f, 0.5f, clip.data(), 2u);
    Require(cache.Images() == 1u && cache.Texts() == 1u,
            "original DLL base draw dispatches to host numbered renderer");
    cache.Clear();
    pal4::detail::NativeString column(module, "");
    Export<void (__thiscall*)(void*, const void*, std::uint32_t, float)>(module,
        "?addColumn@MultiColumnList@CEGUI@@QAEXABVString@2@IM@Z")(list, column.Data(), 0u, 1.0f);
    const auto row = Export<std::uint32_t (__thiscall*)(void*, std::uint32_t)>(module,
        "?addRow@MultiColumnList@CEGUI@@QAEII@Z")(list, 0u);
    Export<void (__thiscall*)(void*, void*, std::uint32_t, std::uint32_t)>(module,
        "?setItem@MultiColumnList@CEGUI@@QAEXPAVListboxItem@2@II@Z")(
            list, reinterpret_cast<void*>(native), 0u, row);
    std::array<std::uint32_t, 13> loaded{};
    loaded[0] = 1u;
    loaded[1] = 1u;
    loaded[3] = 1u;
    loaded[5] = 3u;
    loaded[7] = 7u;
    loaded[9] = 5u;
    loaded[10] = 3661u;
    loaded[11] = 11u;
    loaded[12] = 1u;
    std::string saved_name = "Saved Hero";
    pal4::NativeNumberedListItem detailed(module, data, 27, loaded, 0u, true, saved_name, 20.0f);
    saved_name.assign("changed source");
    Require(runtime.ReadCString(runtime.ReadWord(detailed.Address() + 428u)) == "Saved Hero" &&
            runtime.ReadWord(detailed.Address() + 440u) == 27u &&
            runtime.ReadWord(detailed.Address() + 504u) == data.images.Image("kuang20", "kuang21"),
            "shared native constructor owns name copy, slot and alternate detail image");
    Export<void (__thiscall*)(void*, void*, const void*, float, float, const void*, std::uint32_t)>(module,
        "?draw@ListboxItem@CEGUI@@UBEXAAVRenderCache@2@ABVRect@2@MMPBV42@W4TextFormatting@2@@Z")(
            reinterpret_cast<void*>(detailed.Address()), cache.words.data(), rect.data(), 1.0f, 0.5f, clip.data(), 2u);
    Require(cache.Images() == 3u && cache.Texts() == 7u,
            "constructed loaded item draws actual header, name and icons through DLL virtual call");
    cache.Clear();
    const auto detailed_row = Export<std::uint32_t (__thiscall*)(void*, std::uint32_t)>(module,
        "?addRow@MultiColumnList@CEGUI@@QAEII@Z")(list, 0u);
    Export<void (__thiscall*)(void*, void*, std::uint32_t, std::uint32_t)>(module,
        "?setItem@MultiColumnList@CEGUI@@QAEXPAVListboxItem@2@II@Z")(
            list, reinterpret_cast<void*>(detailed.Address()), 0u, detailed_row);
    Require(owned.Alive() && Export<std::uint32_t (__thiscall*)(const void*)>(module,
            "?getRowCount@MultiColumnList@CEGUI@@QBEIXZ")(list) == 2u,
            "actual MultiColumnList accepts game numbered item and its size virtual");
    Export<void (__thiscall*)(void*)>(module, "?resetList@MultiColumnList@CEGUI@@QAEXXZ")(list);
    Require(!owned.Alive() && owned.Address() == 0u && !detailed.Alive() && detailed.Address() == 0u,
            "original DLL reset auto-deletes native item through host destructor without double release");
    Export<void (__thiscall*)(void*, std::uint32_t)>(module,
        "?removeColumn@MultiColumnList@CEGUI@@QAEXI@Z")(list, 0u);
    std::cout << "Native NumberedListItem shared construction, DLL draw, list insertion and auto-delete verified\n";

    pal4::NativeNumberedListData actual(module, database);
    const auto records = actual.GetGameRecords();
    Require(!records.empty(), "original StoryDesc supplies numbered chapter records");
    const auto& story = database.Tables().at("StoryDesc");
    Require(records.size() == story.rows.size() &&
            records.front().first_text == std::get<std::string>(story.rows.front()[1]) &&
            records.back().second_text == std::get<std::string>(story.rows.back()[2]),
            "chapter provider preserves original record order and text bytes");
    for (unsigned variant = 1u; variant <= 2u; ++variant) {
        const char* inactive[]{"tou1b", "tou2b", "tou4b", "tou3b"};
        const char* active[]{"tou1b_2", "tou2b_2", "tou4b_2", "tou3b"};
        for (unsigned slot = 0u; slot < 4u; ++slot)
            Require(actual.GetIconImage(slot, 6u, variant) ==
                    data.images.Image("gy0", variant == 2u ? active[slot] : inactive[slot]),
                    "save icons use actual DLL images and stored player variant");
    }
    Require(!actual.TriggerObject(0xffffffffu), "missing Scene is not treated as present");
    actual.TriggerPlayer(0xffffffffu);
    for (const auto& invalid : {"%n", "%s", "%*u", "%llu", "%u%u", "%"}) {
        bool rejected = false;
        try { (void)pal4::detail::FormatScalarText(invalid, 1u, true); }
        catch (const std::invalid_argument&) { rejected = true; }
        Require(rejected, "database scalar format rejects incompatible and extra arguments");
    }
    pal4::NativeGameDatabase missing;
    pal4::NativeNumberedListData missing_data(module, missing);
    bool missing_rejected = false;
    try { (void)missing_data.GetGameRecords(); }
    catch (const std::runtime_error&) { missing_rejected = true; }
    Require(missing_rejected, "unloaded story database fails explicitly");

    pal4::NativeGameSaveSummaryReader reader;
    const auto summary = reader.ReadSlot(game_root, 0);
    Require(summary.has_value(), "original save0 summary is available for native row binding");
    const auto header = std::bit_cast<std::array<std::uint32_t, 13>>(summary->header);
    const auto texture_loads = ui.TextureLoads();
    auto thumbnail = ui.LoadSaveThumbnail(0);
    if (!thumbnail) std::cerr << ui.LastError() << '\n';
    Require(thumbnail && ui.TextureLoads() == texture_loads + 1u &&
            data.images.Image("GameSaveImage0", "full_image") == thumbnail,
            "save0 PNG loads through shared imageset loader and original DLL texture virtual");
    const auto thumbnail_width = runtime.ReadFloat(thumbnail + 28u);
    const auto thumbnail_height = runtime.ReadFloat(thumbnail + 32u);
    std::array<unsigned char, 24> png_header{};
    std::ifstream png_file(game_root / "save" / "0.png", std::ios::binary);
    png_file.read(reinterpret_cast<char*>(png_header.data()), png_header.size());
    const auto big_endian = [&](unsigned offset) {
        return (std::uint32_t(png_header[offset]) << 24u) |
            (std::uint32_t(png_header[offset + 1u]) << 16u) |
            (std::uint32_t(png_header[offset + 2u]) << 8u) | png_header[offset + 3u];
    };
    Require(png_file && thumbnail_width == big_endian(16u) && thumbnail_height == big_endian(20u),
            "decoded DLL image dimensions match original PNG IHDR");
    const auto destroyed_before_reload = ui.TextureDestroys();
    thumbnail = ui.LoadSaveThumbnail(0);
    Require(thumbnail && ui.TextureLoads() == texture_loads + 2u &&
            ui.TextureDestroys() == destroyed_before_reload + 1u &&
            runtime.ReadFloat(thumbnail + 28u) == thumbnail_width &&
            runtime.ReadFloat(thumbnail + 32u) == thumbnail_height,
            "thumbnail replacement destroys previous DLL-owned texture and recreates full_image");
    Require(!ui.LoadSaveThumbnail(-1) && !ui.LastError().empty() && !ui.LoadSaveThumbnail(100),
            "thumbnail rejects slots outside original ten-page range");
    for (int slot = 0; slot < 100; ++slot) {
        if (std::filesystem::exists(game_root / "save" / (std::to_string(slot) + ".png"))) continue;
        const auto before_missing = ui.TextureLoads();
        Require(!ui.LoadSaveThumbnail(slot) && !ui.LastError().empty() && ui.TextureLoads() == before_missing,
                "missing save PNG reports failure and cannot become a successful blank thumbnail");
        break;
    }
    pal4::NativeNumberedListItem saved(module, actual, 0, header, thumbnail, true, summary->marker, 100.0f);
    unsigned icon_count = 0u;
    for (unsigned i = 1u; i <= 4u; ++i) {
        if (!header[i]) continue;
        ++icon_count;
        const auto format = database.FindTableTextById("GameString", 201u);
        char expected[128]{};
        Require(format.has_value(), "original GameString201 count format exists");
        const auto length = std::snprintf(expected, sizeof(expected), std::string(*format).c_str(), header[i+4u]);
        Require(length >= 0 && length < sizeof(expected) && actual.FormatItemCount(201u, header[i+4u]) == expected,
                "save player level formatting agrees with original GameString201");
    }
    const bool chapter = std::any_of(records.begin(), records.end(), [&](const auto& record) {
        return record.threshold <= std::bit_cast<std::int32_t>(header[9]);
    });
    Require(actual.TriggerObject(header[11]), "original save scene ID resolves in original Scene table");
    Export<void (__thiscall*)(void*, void*, const void*, float, float, const void*, std::uint32_t)>(module,
        "?draw@ListboxItem@CEGUI@@UBEXAAVRenderCache@2@ABVRect@2@MMPBV42@W4TextFormatting@2@@Z")(
            reinterpret_cast<void*>(saved.Address()), cache.words.data(), rect.data(), 1.0f, 1.0f, clip.data(), 2u);
    Require(cache.Images() == 2u + icon_count && cache.Texts() == 3u + icon_count + (chapter ? 2u : 0u) &&
            runtime.ReadWord(cache.words[1]) == thumbnail,
            "real save header, chapter, level, scene and marker render through DLL base draw");
    cache.Clear();
    std::cout << "Native save thumbnail PNG dimensions, replacement, missing-file failure and row cache verified\n";
    std::cout << "Native NumberedListItem original save summary, database chapters, levels, scenes and icons verified\n";
}
