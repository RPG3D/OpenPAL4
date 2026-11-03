#include "pal4/pal_utilities_table_manager.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUtilitiesTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::vector<std::string> table_events;
    std::map<std::string, std::uint32_t> table_results;
    std::uint32_t ui_result = 0;
    std::uint8_t registration_result = 1;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }

    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        table_events.push_back("write:" + std::to_string(address));
    }

    void RegisterCleanup(pal4::UtilitiesLookupKind kind) override {
        events.push_back(kind == pal4::UtilitiesLookupKind::Tutorial
                             ? "register:tutorial" : "register:game_string");
    }

    void LogError(std::string_view message) override {
        events.push_back("log:" + std::string(message));
    }

    std::uint8_t RegisterTable(std::uint32_t object,
                               std::string_view name,
                               pal4::UtilitiesTableKind kind) override {
        Check(object == 0x9000, "registration receives table object");
        table_events.push_back("register:" + std::string(name) + ":" +
                               std::to_string(static_cast<int>(kind)));
        return kind == pal4::UtilitiesTableKind::MapInfo
                   ? registration_result : 1;
    }

    std::uint32_t LoadTable(std::uint32_t object,
                            std::string_view name) override {
        Check(object == 0x9000, "lookup receives table object");
        table_events.push_back("load:" + std::string(name));
        return table_results.at(std::string(name));
    }

    std::uint32_t GetPropertyString(std::uint32_t object,
                                    std::string_view name) override {
        Check(object == 0x9000 && name == "UI",
              "property lookup receives table object and UI name");
        table_events.push_back("property:UI");
        return ui_result;
    }
};

void SetEntry(Runtime& runtime, std::uint32_t address,
              std::int32_t key, std::uint32_t position) {
    runtime.words[address] = static_cast<std::uint32_t>(key);
    runtime.words[address + 4] = position;
}

void SetTutorialTable(Runtime& runtime) {
    runtime.words[0x101c] = 0x2000;
    runtime.words[0x2008] = 0x3000;
    runtime.words[0x200c] = 0x3020;
    runtime.words[0x2060] = 0x4000;
    SetEntry(runtime, 0x3000, -3, 5);
    SetEntry(runtime, 0x3008, 1, 2);
    SetEntry(runtime, 0x3010, 1, 3);
    SetEntry(runtime, 0x3018, 9, 1);
}

void SetGameStringTable(Runtime& runtime) {
    runtime.words[0x1004] = 0x2400;
    runtime.words[0x2408] = 0x3400;
    runtime.words[0x240c] = 0x3408;
    runtime.words[0x2460] = 0x4400;
    SetEntry(runtime, 0x3400, 42, 4);
}

void SetPreviewTable(Runtime& runtime) {
    runtime.words[0x1008] = 0x2800;
    runtime.words[0x2808] = 0x3800;
    runtime.words[0x280c] = 0x3818;
    runtime.words[0x2860] = 0x4800;
    SetEntry(runtime, 0x3800, -2, 3);
    SetEntry(runtime, 0x3808, 7, 1);
    SetEntry(runtime, 0x3810, 7, 2);
}

void SetIntroductionTable(Runtime& runtime) {
    runtime.words[0x100c] = 0x2c00;
    runtime.words[0x2c08] = 0x3c00;
    runtime.words[0x2c0c] = 0x3c10;
    runtime.words[0x2c60] = 0x4c00;
    SetEntry(runtime, 0x3c00, 4, 2);
    SetEntry(runtime, 0x3c08, 11, 5);
}

void SetSceneTable(Runtime& runtime) {
    runtime.words[0x1014] = 0x5000;
    runtime.words[0x5008] = 0x6000;
    runtime.words[0x500c] = 0x6018;
    runtime.words[0x5060] = 0x7000;
    SetEntry(runtime, 0x6000, -10, 2);
    SetEntry(runtime, 0x6008, 0, 1);
    SetEntry(runtime, 0x6010, 10, 3);
}

void SetMapInfoTable(Runtime& runtime) {
    runtime.words[0x1020] = 0x5400;
    runtime.words[0x5408] = 0x6400;
    runtime.words[0x540c] = 0x6410;
    runtime.words[0x5460] = 0x7400;
    SetEntry(runtime, 0x6400, -1, 4);
    SetEntry(runtime, 0x6408, 5, 2);
}

void ExpectMissing(const std::string& expected,
                   auto&& lookup) {
    try {
        static_cast<void>(lookup());
        Check(false, "missing lookup must throw");
    } catch (const std::runtime_error& error) {
        Check(error.what() == expected, "missing lookup message matches IDA");
    }
}

void CheckSuccessfulLookups() {
    Runtime runtime;
    SetTutorialTable(runtime);
    SetGameStringTable(runtime);
    pal4::UtilitiesLookupErrorState errors;
    pal4::PalUtilitiesTableManager manager(0x1000, errors, runtime);
    Check(manager.FindTutorial(-3) == 0x4068,
          "signed negative key finds its physical row");
    Check(manager.FindTutorial(1) == 0x402c,
          "duplicate key returns the first index entry");
    Check(manager.FindTutorial(9) == 0x4018,
          "last key uses 20-byte row stride and four-byte field offset");
    Check(manager.FindGameString(42) == 0x4454,
          "game string uses manager+4 table and 20-byte row stride");
    Check(runtime.events.empty() && !errors.tutorial.cleanup_registered &&
              !errors.game_string.cleanup_registered,
          "successful lookup never enters the error path");
}

void CheckMissingTutorial() {
    Runtime runtime;
    SetTutorialTable(runtime);
    pal4::UtilitiesLookupErrorState errors;
    pal4::PalUtilitiesTableManager manager(0x1000, errors, runtime);
    const std::string missing =
        "ERROR: can't find the Tutorial, key:0 "
        "<palUtilitiesTableManager::findTutorial>";
    ExpectMissing(missing, [&] { return manager.FindTutorial(0); });
    Check(errors.tutorial.cleanup_registered && errors.tutorial.text == missing &&
              runtime.events == std::vector<std::string>{
                  "register:tutorial", "log:" + missing},
          "first missing tutorial registers cleanup before logging");

    ExpectMissing("ERROR: can't find the Tutorial, key:-4 "
                  "<palUtilitiesTableManager::findTutorial>",
                  [&] { return manager.FindTutorial(-4); });
    ExpectMissing("ERROR: can't find the Tutorial, key:10 "
                  "<palUtilitiesTableManager::findTutorial>",
                  [&] { return manager.FindTutorial(10); });
    Check(runtime.events.size() == 4,
          "gaps and out-of-range keys log without registering twice");
    Check(manager.FindTutorial(1) == 0x402c && runtime.events.size() == 4,
          "successful lookup after a failure retains error state");
}

void CheckEmptyAndNullRecord() {
    Runtime runtime;
    runtime.words[0x101c] = 0x2000;
    runtime.words[0x2008] = 0x3000;
    runtime.words[0x200c] = 0x3000;
    pal4::UtilitiesLookupErrorState errors;
    pal4::PalUtilitiesTableManager manager(0x1000, errors, runtime);
    ExpectMissing("ERROR: can't find the Tutorial, key:7 "
                  "<palUtilitiesTableManager::findTutorial>",
                  [&] { return manager.FindTutorial(7); });
    SetEntry(runtime, 0x3000, 7, 0);
    runtime.words[0x200c] = 0x3008;
    runtime.words[0x2060] = 0;
    ExpectMissing("ERROR: can't find the Tutorial, key:7 "
                  "<palUtilitiesTableManager::findTutorial>",
                  [&] { return manager.FindTutorial(7); });
    Check(runtime.events.size() == 3,
          "empty index and zero record address both take the error path");
}

void CheckGameStringAndCleanup() {
    Runtime runtime;
    SetTutorialTable(runtime);
    SetGameStringTable(runtime);
    pal4::UtilitiesLookupErrorState errors;
    pal4::PalUtilitiesTableManager manager(0x1000, errors, runtime);
    const std::string tutorial =
        "ERROR: can't find the Tutorial, key:2 "
        "<palUtilitiesTableManager::findTutorial>";
    const std::string game_string =
        "ERROR: can't find the gameString, key:-1 "
        "<palUtilitiesTableManager::findGameString>";
    ExpectMissing(tutorial, [&] { return manager.FindTutorial(2); });
    ExpectMissing(game_string, [&] { return manager.FindGameString(-1); });
    Check(runtime.events == std::vector<std::string>{
              "register:tutorial", "log:" + tutorial,
              "register:game_string", "log:" + game_string},
          "tutorial and game string keep independent error channels");

    pal4::PalUtilitiesTableManager::ClearTutorialError(errors);
    Check(errors.tutorial.text.empty() && errors.tutorial.cleanup_registered &&
              errors.game_string.text == game_string,
          "tutorial cleanup clears only its text and preserves guard");
    pal4::PalUtilitiesTableManager::ClearGameStringError(errors);
    Check(errors.game_string.text.empty() &&
              errors.game_string.cleanup_registered,
          "game string cleanup clears its text but retains guard");
    ExpectMissing(tutorial, [&] { return manager.FindTutorial(2); });
    Check(runtime.events.size() == 5 &&
              runtime.events.back() == "log:" + tutorial,
          "lookup after cleanup still skips at-exit registration");
}

void CheckPreviewAndIntroduction() {
    Runtime runtime;
    SetPreviewTable(runtime);
    SetIntroductionTable(runtime);
    pal4::UtilitiesLookupErrorState errors;
    pal4::PalUtilitiesTableManager manager(0x1000, errors, runtime);
    Check(manager.FindPreviewInfo(-2) == 0x489c && manager.HasPreviewInfo(-2),
          "preview lookup uses signed key and 52-byte record stride");
    Check(manager.FindPreviewInfo(7) == 0x4834 && manager.HasPreviewInfo(7),
          "preview lower bound chooses first duplicate");
    Check(manager.FindIntroduction(4) == 0x4c48 &&
              manager.HasIntroduction(4),
          "introduction lookup uses manager+12 and 36-byte stride");
    Check(manager.FindIntroduction(11) == 0x4cb4 &&
              manager.HasIntroduction(11),
          "introduction last key chooses its indexed row");
    Check(manager.FindPreviewInfo(6) == 0 && !manager.HasPreviewInfo(6) &&
              manager.FindIntroduction(12) == 0 &&
              !manager.HasIntroduction(12),
          "absent keys return zero or false without error logging");
    Check(runtime.events.empty(), "non-throwing queries never log errors");

    runtime.words[0x2860] = 0;
    SetEntry(runtime, 0x3800, -2, 0);
    Check(manager.FindPreviewInfo(-2) == 0 && !manager.HasPreviewInfo(-2),
          "zero preview row address is treated as absent by existence test");
    runtime.words[0x2c08] = runtime.words[0x2c0c];
    Check(manager.FindIntroduction(4) == 0 &&
              !manager.HasIntroduction(4),
          "empty introduction index returns zero and false");
}

void CheckSceneAndMapInfo() {
    Runtime runtime;
    SetSceneTable(runtime);
    SetMapInfoTable(runtime);
    pal4::UtilitiesLookupErrorState errors;
    pal4::PalUtilitiesTableManager manager(0x1000, errors, runtime);
    Check(manager.FindScene(-10) == 0x7078 && manager.HasScene(-10) &&
              manager.FindScene(0) == 0x703c && manager.HasScene(0) &&
              manager.FindScene(10) == 0x70b4,
          "scene uses manager+20, signed keys and 60-byte rows");
    Check(manager.FindMapInfo(-1) == 0x7420 &&
              manager.FindMapInfo(5) == 0x7410,
          "map info uses manager+32 and eight-byte rows");
    Check(manager.FindScene(-11) == 0 && !manager.HasScene(-11) &&
              manager.FindScene(11) == 0 && !manager.HasScene(11) &&
              manager.FindMapInfo(0) == 0,
          "scene and map info misses return zero or false");
    Check(runtime.events.empty(), "non-throwing table lookups do not log");

    runtime.words[0x5060] = 0;
    SetEntry(runtime, 0x6000, -10, 0);
    Check(manager.FindScene(-10) == 0 && !manager.HasScene(-10),
          "zero scene row is absent for both lookup variants");
    runtime.words[0x5408] = runtime.words[0x540c];
    Check(manager.FindMapInfo(5) == 0,
          "empty map info index returns zero");
}

void SetNamedTables(Runtime& runtime) {
    runtime.words[0x1000] = 0x9000;
    runtime.table_results = {
        {"GameString", 0xa100}, {"Tutorial", 0xa200},
        {"MapInfo", 0xa300}, {"PreviewInfo", 0xa400},
        {"Introduction", 0xa500}, {"HelpTips", 0xa600},
        {"Scene", 0xa700}, {"CombatScene", 0xa800},
    };
    runtime.ui_result = 0xb000;
}

void CheckUiEnumAndRegistration() {
    Runtime runtime;
    SetNamedTables(runtime);
    pal4::UtilitiesLookupErrorState errors;
    pal4::PalUtilitiesTableManager manager(0x1000, errors, runtime);
    Check(manager.GetUiEnum() == 0xb000 &&
              runtime.table_events == std::vector<std::string>{"property:UI"},
          "UI property query forwards its native pointer");
    runtime.table_events.clear();
    runtime.registration_result = 0;
    Check(manager.RegisterTables() == 0 &&
              runtime.table_events == std::vector<std::string>{
                  "register:GameString:0", "register:PreviewInfo:1",
                  "register:Introduction:2", "register:HelpTips:3",
                  "register:Scene:4", "register:CombatScene:5",
                  "register:Tutorial:6", "register:MapInfo:7"},
          "eight registration calls preserve names, parser identities and last return");
}

void CheckVerifiedTables() {
    Runtime runtime;
    SetNamedTables(runtime);
    pal4::UtilitiesLookupErrorState errors;
    pal4::PalUtilitiesTableManager manager(0x1000, errors, runtime);
    Check(manager.VerifyDataTables() == 1 &&
              runtime.table_events == std::vector<std::string>{
                  "load:GameString", "write:4100",
                  "load:Tutorial", "write:4124",
                  "load:MapInfo", "write:4128",
                  "load:PreviewInfo", "write:4104",
                  "load:Introduction", "write:4108",
                  "load:HelpTips", "write:4112",
                  "load:Scene", "write:4116",
                  "property:UI", "load:CombatScene", "write:4120"},
          "verification loads tables in IDA order with the UI gate before CombatScene");
    Check(runtime.words[0x1004] == 0xa100 &&
              runtime.words[0x101c] == 0xa200 &&
              runtime.words[0x1020] == 0xa300 &&
              runtime.words[0x1008] == 0xa400 &&
              runtime.words[0x100c] == 0xa500 &&
              runtime.words[0x1010] == 0xa600 &&
              runtime.words[0x1014] == 0xa700 &&
              runtime.words[0x1018] == 0xa800 && runtime.events.empty(),
          "all eight table pointers occupy their observed manager slots");
}

void CheckVerificationFailures() {
    constexpr const char* required[] = {
        "GameString", "MapInfo", "PreviewInfo", "Introduction",
        "HelpTips", "Scene", "CombatScene"};
    for (const auto* missing : required) {
        Runtime runtime;
        SetNamedTables(runtime);
        runtime.table_results[missing] = 0;
        pal4::UtilitiesLookupErrorState errors;
        pal4::PalUtilitiesTableManager manager(0x1000, errors, runtime);
        const auto message =
            "ERROR [ the table " + std::string(missing) +
            "  doesn't exist <palUtilitiesTableManager::verifyDataTable >]";
        ExpectMissing(message, [&] { return manager.VerifyDataTables(); });
        Check(runtime.events == std::vector<std::string>{"log:" + message},
              "missing required table logs its exact error once");
    }

    Runtime tutorial_runtime;
    SetNamedTables(tutorial_runtime);
    tutorial_runtime.table_results["Tutorial"] = 0;
    pal4::UtilitiesLookupErrorState tutorial_errors;
    pal4::PalUtilitiesTableManager tutorial_manager(
        0x1000, tutorial_errors, tutorial_runtime);
    Check(tutorial_manager.VerifyDataTables() == 1 &&
              tutorial_runtime.words[0x101c] == 0 &&
              tutorial_runtime.events.empty(),
          "original Tutorial check reads GameString slot and lets null Tutorial pass");

    Runtime ui_runtime;
    SetNamedTables(ui_runtime);
    ui_runtime.ui_result = 0;
    pal4::UtilitiesLookupErrorState ui_errors;
    pal4::PalUtilitiesTableManager ui_manager(0x1000, ui_errors,
                                                ui_runtime);
    ExpectMissing("UI enum doesn't exist",
                  [&] { return ui_manager.VerifyDataTables(); });
    Check(ui_runtime.events ==
              std::vector<std::string>{"log:UI enum doesn't exist"} &&
              ui_runtime.table_events.back() == "property:UI",
          "missing UI property logs and throws before CombatScene lookup");
}

}  // namespace

int main() {
    try {
        CheckSuccessfulLookups();
        CheckMissingTutorial();
        CheckEmptyAndNullRecord();
        CheckGameStringAndCleanup();
        CheckPreviewAndIntroduction();
        CheckSceneAndMapInfo();
        CheckUiEnumAndRegistration();
        CheckVerifiedTables();
        CheckVerificationFailures();
        std::cout << "pal_utilities_table_manager_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
