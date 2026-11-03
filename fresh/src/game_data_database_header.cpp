#include "pal4/game_data_database_header.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string>

namespace pal4 {

GameDataDatabaseHeader& GameDataDatabaseHeader::Reset() {
    words_[34] = 0;
    std::fill_n(words_.begin(), 32, 0);
    return *this;
}

namespace {

int X87RangeMask(float first, float second) {
    if (std::isunordered(first, second)) return 0x4100;
    if (first < second) return 0x0100;
    if (first == second) return 0x4000;
    return 0;
}

}  // namespace

int GameDataDatabaseHeader::LoadCombatFormulas(
    IGameDataCombatFormulaRuntime& runtime) {
    constexpr std::string_view kTableName = "CombatFormulaConfig";
    constexpr std::string_view kMissingTable =
        "ERROR [ dbCombatSetting::loadSettings \t\t\t< can't find the combatFormulaConfig Table!>]";
    if (!runtime.OpenTable(kTableName)) {
        runtime.LogError(kMissingTable);
        throw std::runtime_error(std::string(kMissingTable));
    }

    for (std::uint32_t key = 1; key <= 32; ++key) {
        const auto keys = runtime.Keys();
        const auto position = GameDataAiKeyAlgorithms::LowerBound(keys, key);
        if (position == keys.size() || keys[position].key != key ||
            !runtime.HasRow(keys[position].record_index)) {
            const std::string message =
                "ERROR [dbCombatSetting::loadSettings \t\t\t\tcan't find the dbRow,key:" +
                std::to_string(key);
            runtime.LogError(message);
            throw std::runtime_error(message);
        }
        words_[key - 1] = std::bit_cast<std::uint32_t>(
            runtime.ReadFloatProperty(keys[position].record_index, 1));
    }

    const auto value = [&](std::size_t index) {
        return std::bit_cast<float>(words_[index]);
    };
    if (X87RangeMask(value(1), value(2)) == 0)
        (void)runtime.LogRangeWarning(0x73, "physical attack param2 data abnormal!!");
    const auto magic_mask = X87RangeMask(value(6), value(7));
    if (magic_mask == 0)
        return runtime.LogRangeWarning(0x7f,
                                       "magic attack param2 data abnormal!!");
    return magic_mask;
}

double GameDataDatabaseHeader::FloatAt(std::int32_t index) const {
    if (index < 0 || index >= 32) return 0.0;
    return std::bit_cast<float>(words_[static_cast<std::size_t>(index)]);
}

std::uint32_t GameDataDatabaseHeader::SetFactoryRegistry(
    std::uint32_t registry) {
    words_[34] = registry;
    return registry;
}

std::uint8_t GameDataDatabaseHeader::RegisterFactories(
    IGameDataDatabaseFactoryRuntime& runtime) const {
    (void)runtime.RegisterFactory(words_[34], "UpgradeParam", 0x4cfee0);
    return static_cast<std::uint8_t>(
        runtime.RegisterFactory(words_[34], "DropProperty", 0x4d0910));
}

std::uint8_t GameDataDatabaseHeader::InitializeFromDatabase(
    IGameDataDatabaseInitializationRuntime& runtime) {
    (void)LoadCombatFormulas(runtime);
    const auto require_factory = [&](std::string_view name,
                                     std::uint32_t& destination) {
        destination = runtime.FindFactory(words_[34], name);
        if (destination == 0) {
            const std::string message =
                "ERROR [ dbCombatSetting::verifyDataTable < the table " +
                std::string(name) + "  doesn't exist>]";
            runtime.LogError(message);
            throw std::runtime_error(message);
        }
    };
    require_factory("UpgradeParam", words_[32]);
    require_factory("DropProperty", words_[33]);
    return 1;
}

}  // namespace pal4
