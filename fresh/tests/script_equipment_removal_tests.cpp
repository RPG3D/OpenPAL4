#include "pal4/script_equipment_removal.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Item {
    bool present = true;
    std::uint32_t id = static_cast<std::uint32_t>(-7);
    bool condition = false;
};

struct Runtime final : pal4::IScriptEquipmentRemovalRuntime {
    std::vector<std::string> calls;
    std::vector<Item> items;
    std::string fail_at;
    std::uint32_t hint = 0x5000u;
    bool has_record = true;
    std::int32_t count_override = -999;
    std::uint32_t name = 0x9000u;
    std::uint32_t format = 0xa000u;
    std::uint8_t expected_hint_flag = 1u;
    std::uint32_t next_text = 0x7000u;

    void Record(std::string call) {
        calls.push_back(call);
        if (call == fail_at) throw std::runtime_error("injected failure");
    }
    std::uint32_t FrameManager() override {
        Record("frames");
        return 0x4000u;
    }
    std::uint32_t CreateHintWindow(std::uint32_t manager) override {
        Check(manager == 0x4000u, "hint window manager");
        Record("hint");
        return hint;
    }
    std::uint32_t GameDatabase() override {
        Record("database");
        return 0x1000u;
    }
    bool HasRecord(std::uint32_t table, std::int32_t id) override {
        Check(table == 0x1058u && id == -7, "equipment record gate");
        Record("has");
        return has_record;
    }
    std::uint32_t FindRecord(std::uint32_t table,
                             std::int32_t id) override {
        Check(table == 0x1058u && id == -7, "equipment record lookup");
        Record("record");
        return 0x3000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x3094u) {
            Record("category");
            return 7u;
        }
        if (address == 0x309cu) {
            Record("name");
            return name;
        }
        if (address == 0x6004u) {
            Record("format-pointer");
            return format;
        }
        for (std::size_t index = 0; index < items.size(); ++index) {
            if (address == 0x5090u + index * 0x100u) {
                Record("item-id:" + std::to_string(index));
                return items[index].id;
            }
        }
        throw std::runtime_error("unexpected equipment field");
    }
    std::uint32_t Inventory() override {
        Record("inventory");
        return 0x2000u;
    }
    std::int32_t ElementCount(std::uint32_t collection,
                              std::uint32_t category) override {
        Check(collection == 0x207cu && category == 7u,
              "equipment collection count operands");
        Record("count");
        return count_override == -999 ?
            static_cast<std::int32_t>(items.size()) : count_override;
    }
    std::uint32_t ElementAt(std::uint32_t collection,
                            std::uint32_t category,
                            std::uint32_t index) override {
        Check(collection == 0x207cu && category == 7u &&
                  index < items.size(), "equipment element lookup");
        Record("item:" + std::to_string(index));
        return items[index].present ? 0x5000u + index * 0x100u : 0u;
    }
    bool CheckPlayersCondition(std::uint32_t element) override {
        const auto index = (element - 0x5000u) / 0x100u;
        Check(index < items.size() &&
                  element == 0x5000u + index * 0x100u,
              "player condition element");
        Record("condition:" + std::to_string(index));
        return items[index].condition;
    }
    void RemoveElement(std::uint32_t collection,
                       std::uint32_t element) override {
        Check(collection == 0x207cu && element >= 0x5000u,
              "equipment removal operands");
        Record("remove");
    }
    std::uint32_t InitializeText() override {
        const auto text = next_text;
        next_text += 0x100u;
        Record("text-init");
        return text;
    }
    std::uint32_t EmptyText() override {
        Record("empty");
        return 0xe000u;
    }
    std::uint32_t FindGameString(std::uint32_t table,
                                 std::uint32_t id) override {
        Check(table == 0x1034u && id == 0x7eu,
              "localized equipment hint string");
        Record("find-string");
        return 0x6000u;
    }
    void FormatText(std::uint32_t destination,
                    std::uint32_t text_format,
                    std::uint32_t text_name) override {
        Check(destination == 0x7100u &&
                  text_format == (format ? format : 0xe000u) &&
                  text_name == (name ? name : 0xe000u),
              "equipment hint format and name");
        Record("format");
    }
    void AppendText(std::uint32_t destination,
                    std::uint32_t source) override {
        Check(destination == 0x7100u && source == 0x7000u,
              "hint text append operands");
        Record("append");
    }
    void ShowHint(std::uint32_t window, std::uint32_t text,
                  float x, float y, float scale,
                  std::uint32_t mode, std::uint32_t visible) override {
        Check(window == 0x5000u && text == 0x7100u &&
                  x == 0.5f && y == 0.15f && scale == 1.0f &&
                  mode == 0u && visible == 1u,
              "equipment hint geometry and flags");
        Record("show");
    }
    std::uint32_t AudioManager() override {
        Record("audio");
        return 0x8000u;
    }
    void PlayMusic(std::uint32_t audio, std::uint8_t flag,
                   std::string_view path, std::uint32_t mode) override {
        Check(audio == 0x8000u && flag == expected_hint_flag &&
                  path == "gamedata\\PALSound\\WA07.wav" && mode == 1u,
              "equipment removal sound");
        Record("music");
    }
    std::uint8_t CleanupText(std::uint32_t text) override {
        Check(text == 0x7000u || text == 0x7100u,
              "temporary text cleanup handle");
        Record(text == 0x7100u ? "cleanup-format" : "cleanup-source");
        return text == 0x7000u ? 0x34u : 0x56u;
    }
};

void EarlyReturns() {
    Runtime no_hint;
    no_hint.hint = 0u;
    Check(pal4::ScriptEquipmentRemoval::Remove(-7, 1u, no_hint) == 0u &&
              no_hint.calls == std::vector<std::string>{"frames", "hint"},
          "missing hint window skips database and inventory");
    Runtime no_record;
    no_record.has_record = false;
    Check(pal4::ScriptEquipmentRemoval::Remove(-7, 1u, no_record) == 0u &&
              no_record.calls == std::vector<std::string>{
                  "frames", "hint", "database", "has"},
          "missing GameDB key skips inventory");
    for (auto count : {0, -1}) {
        Runtime empty;
        empty.count_override = count;
        Check(pal4::ScriptEquipmentRemoval::Remove(-7, 1u, empty) ==
                  static_cast<std::uint8_t>(count) &&
                  empty.calls == std::vector<std::string>{
                      "frames", "hint", "database", "has", "database",
                      "record", "category", "inventory", "count"},
              "nonpositive count returns original low byte");
    }
}

void ScanAndRemove() {
    Runtime miss;
    miss.items = {{false}, {true, 0x33u},
                  {true, static_cast<std::uint32_t>(-7), true}};
    Check(pal4::ScriptEquipmentRemoval::Remove(-7, 0u, miss) == 1u &&
              miss.calls.back() == "condition:2",
          "exhausted scan forwards last condition byte");
    Runtime absent;
    absent.items = {{false}};
    Check(pal4::ScriptEquipmentRemoval::Remove(-7, 0u, absent) == 0u &&
              absent.calls.back() == "item:0",
          "null element skips ID and condition");
    Runtime mismatch;
    mismatch.items = {{true, 0x33u}};
    Check(pal4::ScriptEquipmentRemoval::Remove(-7, 0u, mismatch) ==
                  static_cast<std::uint8_t>(-7) &&
              mismatch.calls.back() == "item-id:0",
          "mismatch exhaustion keeps script ID low byte");
    Runtime removed;
    removed.items = {{true, static_cast<std::uint32_t>(-7), false}};
    Check(pal4::ScriptEquipmentRemoval::Remove(-7, 0u, removed) == 0u &&
              removed.calls[removed.calls.size() - 2u] == "inventory" &&
              removed.calls.back() == "remove",
          "matching usable equipment removed without hint");
}

void HintAndCleanup() {
    Runtime runtime;
    runtime.items = {{true, static_cast<std::uint32_t>(-7), false}};
    runtime.name = 0u;
    runtime.format = 0u;
    runtime.expected_hint_flag = 0xffu;
    Check(pal4::ScriptEquipmentRemoval::Remove(-7, 0xffu, runtime) == 0x34u,
          "hint path forwards final source cleanup byte");
    Check(runtime.calls == std::vector<std::string>{
              "frames", "hint", "database", "has", "database", "record",
              "category", "inventory", "count", "category", "inventory",
              "item:0", "item-id:0", "condition:0", "inventory", "remove",
              "text-init", "text-init", "name", "empty", "database",
              "find-string", "format-pointer", "empty", "format", "append",
              "show", "audio", "music", "cleanup-format", "cleanup-source"},
          "hint formatting, sound and reverse cleanup order");
    for (const auto* fail : {"format", "append", "show", "music"}) {
        Runtime failing;
        failing.items = {{true, static_cast<std::uint32_t>(-7), false}};
        failing.fail_at = fail;
        bool caught = false;
        try {
            (void)pal4::ScriptEquipmentRemoval::Remove(-7, 1u, failing);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && failing.calls[failing.calls.size() - 2u] ==
                            "cleanup-format" &&
                  failing.calls.back() == "cleanup-source",
              "hint failure cleans both temporary strings");
    }
}

}  // namespace

int main() {
    try {
        EarlyReturns();
        ScanAndRemove();
        HintAndCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
