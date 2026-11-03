#include "pal4/pal_utilities_table_manager.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace pal4 {

std::uint32_t PalUtilitiesTableManager::FindTutorial(std::int32_t key) {
    const auto row = FindIndexedRow(28, key, 20);
    if (row != 0) return row + 4;
    ReportMissing(key, UtilitiesLookupKind::Tutorial);
}

std::uint32_t PalUtilitiesTableManager::FindGameString(std::int32_t key) {
    const auto row = FindIndexedRow(4, key, 20);
    if (row != 0) return row + 4;
    ReportMissing(key, UtilitiesLookupKind::GameString);
}

std::uint32_t PalUtilitiesTableManager::FindPreviewInfo(std::int32_t key) {
    return FindIndexedRow(8, key, 52);
}

bool PalUtilitiesTableManager::HasPreviewInfo(std::int32_t key) {
    return FindIndexedRow(8, key, 52) != 0;
}

std::uint32_t PalUtilitiesTableManager::FindIntroduction(std::int32_t key) {
    return FindIndexedRow(12, key, 36);
}

bool PalUtilitiesTableManager::HasIntroduction(std::int32_t key) {
    return FindIndexedRow(12, key, 36) != 0;
}

std::uint32_t PalUtilitiesTableManager::FindScene(std::int32_t key) {
    return FindIndexedRow(20, key, 60);
}

bool PalUtilitiesTableManager::HasScene(std::int32_t key) {
    return FindIndexedRow(20, key, 60) != 0;
}

std::uint32_t PalUtilitiesTableManager::FindMapInfo(std::int32_t key) {
    return FindIndexedRow(32, key, 8);
}

std::uint32_t PalUtilitiesTableManager::GetUiEnum() {
    return runtime_.GetPropertyString(runtime_.ReadWord(address_), "UI");
}

std::uint8_t PalUtilitiesTableManager::RegisterTables() {
    constexpr std::array registrations = {
        std::pair{"GameString", UtilitiesTableKind::GameString},
        std::pair{"PreviewInfo", UtilitiesTableKind::PreviewInfo},
        std::pair{"Introduction", UtilitiesTableKind::Introduction},
        std::pair{"HelpTips", UtilitiesTableKind::HelpTips},
        std::pair{"Scene", UtilitiesTableKind::Scene},
        std::pair{"CombatScene", UtilitiesTableKind::CombatScene},
        std::pair{"Tutorial", UtilitiesTableKind::Tutorial},
        std::pair{"MapInfo", UtilitiesTableKind::MapInfo},
    };
    const auto object = runtime_.ReadWord(address_);
    std::uint8_t result = 0;
    for (const auto& [name, kind] : registrations) {
        result = runtime_.RegisterTable(object, name, kind);
    }
    return result;
}

void PalUtilitiesTableManager::LoadRequiredTable(
    std::string_view name, std::uint32_t slot_offset) {
    const auto table = runtime_.LoadTable(runtime_.ReadWord(address_), name);
    runtime_.WriteWord(address_ + slot_offset, table);
    if (table == 0) ReportMissingTable(name);
}

void PalUtilitiesTableManager::ReportMissingTable(std::string_view name) {
    const auto message =
        "ERROR [ the table " + std::string(name) +
        "  doesn't exist <palUtilitiesTableManager::verifyDataTable >]";
    runtime_.LogError(message);
    throw std::runtime_error(message);
}

std::uint8_t PalUtilitiesTableManager::VerifyDataTables() {
    LoadRequiredTable("GameString", 4);

    const auto tutorial = runtime_.LoadTable(runtime_.ReadWord(address_),
                                              "Tutorial");
    runtime_.WriteWord(address_ + 28, tutorial);
    // The original checks GameString again here, leaving a null Tutorial slot.
    if (runtime_.ReadWord(address_ + 4) == 0) {
        ReportMissingTable("Tutorial");
    }

    LoadRequiredTable("MapInfo", 32);
    LoadRequiredTable("PreviewInfo", 8);
    LoadRequiredTable("Introduction", 12);
    LoadRequiredTable("HelpTips", 16);
    LoadRequiredTable("Scene", 20);
    if (GetUiEnum() == 0) {
        constexpr auto message = "UI enum doesn't exist";
        runtime_.LogError(message);
        throw std::runtime_error(message);
    }
    LoadRequiredTable("CombatScene", 24);
    return 1;
}

std::uint32_t PalUtilitiesTableManager::FindIndexedRow(
    std::uint32_t table_offset, std::int32_t key,
    std::uint32_t record_width) {
    const auto table = runtime_.ReadWord(address_ + table_offset);
    auto cursor = runtime_.ReadWord(table + 8);
    const auto end = runtime_.ReadWord(table + 12);
    auto count = static_cast<std::int32_t>(end - cursor) >> 3;

    while (count > 0) {
        const auto middle = count / 2;
        const auto entry = cursor + 8u * static_cast<std::uint32_t>(middle);
        if (static_cast<std::int32_t>(runtime_.ReadWord(entry)) >= key) {
            count = middle;
        } else {
            cursor = entry + 8;
            count -= middle + 1;
        }
    }

    if (cursor != end &&
        key >= static_cast<std::int32_t>(runtime_.ReadWord(cursor))) {
        const auto position = runtime_.ReadWord(cursor + 4);
        return runtime_.ReadWord(table + 96) + record_width * position;
    }
    return 0;
}

void PalUtilitiesTableManager::ReportMissing(
    std::int32_t key, UtilitiesLookupKind kind) {
    auto& slot = kind == UtilitiesLookupKind::Tutorial
                     ? errors_.tutorial : errors_.game_string;
    if (!slot.cleanup_registered) {
        slot.cleanup_registered = true;
        slot.text.clear();
        runtime_.RegisterCleanup(kind);
    }
    const auto prefix = kind == UtilitiesLookupKind::Tutorial
                            ? "ERROR: can't find the Tutorial, key:"
                            : "ERROR: can't find the gameString, key:";
    const auto suffix = kind == UtilitiesLookupKind::Tutorial
                            ? " <palUtilitiesTableManager::findTutorial>"
                            : " <palUtilitiesTableManager::findGameString>";
    std::string message = prefix + std::to_string(key) + suffix;
    runtime_.LogError(message);
    slot.text = message;
    throw std::runtime_error(slot.text);
}

void PalUtilitiesTableManager::ClearTutorialError(
    UtilitiesLookupErrorState& errors) {
    std::string{}.swap(errors.tutorial.text);
}

void PalUtilitiesTableManager::ClearGameStringError(
    UtilitiesLookupErrorState& errors) {
    std::string{}.swap(errors.game_string.text);
}

}  // namespace pal4
