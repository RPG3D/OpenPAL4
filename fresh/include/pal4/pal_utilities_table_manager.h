#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

enum class UtilitiesLookupKind { Tutorial, GameString };
enum class UtilitiesTableKind {
    GameString, PreviewInfo, Introduction, HelpTips,
    Scene, CombatScene, Tutorial, MapInfo
};

struct UtilitiesLookupErrorSlot {
    bool cleanup_registered = false;
    std::string text;
};

struct UtilitiesLookupErrorState {
    UtilitiesLookupErrorSlot tutorial;
    UtilitiesLookupErrorSlot game_string;
};

class IUtilitiesTableRuntime {
public:
    virtual ~IUtilitiesTableRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void RegisterCleanup(UtilitiesLookupKind kind) = 0;
    virtual void LogError(std::string_view message) = 0;
    virtual std::uint8_t RegisterTable(std::uint32_t object,
                                        std::string_view name,
                                        UtilitiesTableKind kind) = 0;
    virtual std::uint32_t LoadTable(std::uint32_t object,
                                     std::string_view name) = 0;
    virtual std::uint32_t GetPropertyString(std::uint32_t object,
                                             std::string_view name) = 0;
};

class PalUtilitiesTableManager final {
public:
    PalUtilitiesTableManager(std::uint32_t address,
                             UtilitiesLookupErrorState& errors,
                             IUtilitiesTableRuntime& runtime)
        : address_(address), errors_(errors), runtime_(runtime) {}

    // IDA 0x500cd0 and 0x500e90.
    [[nodiscard]] std::uint32_t FindTutorial(std::int32_t key);
    [[nodiscard]] std::uint32_t FindGameString(std::int32_t key);
    // IDA 0x501050-0x5011b0: non-throwing preview/intro record queries.
    [[nodiscard]] std::uint32_t FindPreviewInfo(std::int32_t key);
    [[nodiscard]] bool HasPreviewInfo(std::int32_t key);
    [[nodiscard]] std::uint32_t FindIntroduction(std::int32_t key);
    [[nodiscard]] bool HasIntroduction(std::int32_t key);
    // IDA 0x501230-0x501320: Scene and MapInfo index queries.
    [[nodiscard]] std::uint32_t FindScene(std::int32_t key);
    [[nodiscard]] bool HasScene(std::int32_t key);
    [[nodiscard]] std::uint32_t FindMapInfo(std::int32_t key);
    // IDA 0x501390-0x501440: register and resolve eight named data tables.
    [[nodiscard]] std::uint32_t GetUiEnum();
    [[nodiscard]] std::uint8_t RegisterTables();
    [[nodiscard]] std::uint8_t VerifyDataTables();

    // IDA 0x500e40 and 0x501000: the at-exit handlers retain their guard bits.
    static void ClearTutorialError(UtilitiesLookupErrorState& errors);
    static void ClearGameStringError(UtilitiesLookupErrorState& errors);

private:
    [[nodiscard]] std::uint32_t FindIndexedRow(
        std::uint32_t table_offset, std::int32_t key,
        std::uint32_t record_width);
    [[noreturn]] void ReportMissing(std::int32_t key,
                                    UtilitiesLookupKind kind);
    void LoadRequiredTable(std::string_view name, std::uint32_t slot_offset);
    [[noreturn]] void ReportMissingTable(std::string_view name);

    std::uint32_t address_;
    UtilitiesLookupErrorState& errors_;
    IUtilitiesTableRuntime& runtime_;
};

}  // namespace pal4
