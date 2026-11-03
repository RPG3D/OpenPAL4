#include "pal4/game_data_monster_table.h"

#include <cstdint>
#include <algorithm>
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

struct Runtime final : pal4::IGameDataMonsterTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t found_table = 0x2000;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address));
        words[address] = value;
    }
    std::uint8_t PreloadTable(std::uint32_t database, std::string_view name,
                              std::uint32_t factory) override {
        Check(database == 0x8000 && name == "Monster" &&
                  factory == 0x4debd0,
              "preload forwards database, literal table and factory");
        calls.emplace_back("preload");
        return 0x72;
    }
    std::uint32_t FindTable(std::uint32_t database,
                            std::string_view name) override {
        Check(database == 0x8000 && name == "Monster",
              "verify looks up the Monster table");
        calls.emplace_back("find");
        return found_table;
    }
    void LogError(std::string_view message) override {
        Check(message ==
                  "ERROR [ palDBMonsterManager::verifyDataTable < the table "
                  "Monster  doesn't exist>]",
              "missing Monster table log text matches IDA format");
        calls.emplace_back("log");
    }
};

struct RowRuntime final : pal4::IGameDataMonsterRowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t bound_count = 1;
    std::uint32_t found_position = 0x3000;
    std::uint32_t parsed_key = static_cast<std::uint32_t>(-5);
    bool parse_result = true;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x1000 + 124, "binding collection address");
        calls.emplace_back("size");
        return bound_count;
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t table_name) override {
        Check(table_name == 0x1000 + 52, "bind log uses table name");
        calls.push_back(std::string(prefix));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x1000 + 108 && source == 0x1000 + 20,
              "Effect binding addresses");
        calls.emplace_back("assign effects");
    }
    void InitializeStructures(std::uint32_t binding) override {
        Check(binding == 0x1000 + 108, "binding initialization address");
        calls.emplace_back("init binding");
    }
    void BindEffects(std::uint32_t binding) override {
        Check(binding == 0x1000 + 108, "Effect bind address");
        calls.emplace_back("bind effects");
    }
    void InitializeRecord(pal4::GameDataMonsterRecord& record) override {
        record.fill(0);
        calls.emplace_back("init record");
    }
    void DestroyTemporaryRecordTexts(
        pal4::GameDataMonsterRecord& record) override {
        Check(record[100] == parsed_key || !parse_result,
              "temporary record key persists until cleanup");
        calls.emplace_back("destroy texts");
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::GameDataMonsterRecord& record) override {
        Check(binding == 0x1000 + 108 && row == 0x6000,
              "parser receives binding and row");
        calls.emplace_back("parse");
        record[100] = parsed_key;
        return parse_result;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t key) override {
        Check(begin == 0x3000 && end == 0x3008 && key == parsed_key,
              "index search uses exact range and record offset +400 key");
        calls.emplace_back("find key");
        return found_position;
    }
    void CopyRecord(std::uint32_t destination,
                    const pal4::GameDataMonsterRecord& record) override {
        Check(destination == 0x4000 + 2 * 596 &&
                  record[100] == parsed_key,
              "existing indexed record receives 596-byte copy");
        calls.emplace_back("copy record");
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataMonsterRecord& record) override {
        Check(array == 0x1000 + 92 && end == 0x4000 + 3 * 596 &&
                  record[100] == parsed_key,
              "new Monster record appends at record-array end");
        calls.emplace_back("insert record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t position,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1000 + 4 && position == found_position &&
                  key == parsed_key && index == 3,
              "new key uses current 596-byte record index");
        calls.emplace_back("insert key");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x6000,
              "failure dump receives table and row");
        calls.emplace_back("dump");
        return " [row 7]";
    }
};

std::size_t At(const RowRuntime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected Monster row operation missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

RowRuntime MakeRowRuntime() {
    RowRuntime runtime;
    runtime.words[0x1000 + 24] = 0x8000;
    runtime.words[0x1000 + 28] = 0x8000 + 2 * 60;
    runtime.words[0x1000 + 8] = 0x3000;
    runtime.words[0x1000 + 12] = 0x3008;
    runtime.words[0x3000] = static_cast<std::uint32_t>(-5);
    runtime.words[0x3004] = 2;
    runtime.words[0x1000 + 96] = 0x4000;
    runtime.words[0x1000 + 100] = 0x4000 + 3 * 596;
    return runtime;
}

void CheckAddOrReplace() {
    auto replaced = MakeRowRuntime();
    Check(pal4::GameDataMonsterTable::AddOrReplace(
              0x1000, 0x6000, replaced) == 1 &&
              At(replaced, "beginBindField, table:") <
                  At(replaced, "assign effects") &&
              At(replaced, "assign effects") <
                  At(replaced, "init binding") &&
              At(replaced, "init binding") <
                  At(replaced, "bind effects") &&
              At(replaced, "bind effects") <
                  At(replaced, "endBindField, table:") &&
              At(replaced, "parse") < At(replaced, "copy record") &&
              replaced.calls.back() == "destroy texts",
          "binding mismatch rebuilds before indexed replacement and cleanup");

    auto inserted = MakeRowRuntime();
    inserted.bound_count = 2;
    inserted.found_position = 0x3008;
    inserted.parsed_key = 7;
    Check(pal4::GameDataMonsterTable::AddOrReplace(
              0x1000, 0x6000, inserted) == 1 &&
              At(inserted, "insert record") < At(inserted, "insert key") &&
              inserted.calls.back() == "destroy texts" &&
              std::find(inserted.calls.begin(), inserted.calls.end(),
                        "assign effects") == inserted.calls.end(),
          "missing key appends record then key without redundant binding");

    auto failed = MakeRowRuntime();
    failed.parse_result = false;
    bool threw = false;
    try {
        (void)pal4::GameDataMonsterTable::AddOrReplace(
            0x1000, 0x6000, failed);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) ==
                "ERROR <dbRecordBindTable< Record >::addRow > [row 7]";
    }
    Check(threw && At(failed, "parse") < At(failed, "dump") &&
              failed.calls.back() == "destroy texts",
          "parse failure dumps row, throws and destroys temporary texts");
}

struct AppendRuntime final : pal4::IGameDataMonsterAppendRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t row_size = 2;
    std::uint32_t bound_count = 2;
    std::uint32_t conversion_count = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x1000 + 124, "append binding collection address");
        calls.emplace_back("size");
        return bound_count;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x6000, "append row address");
        calls.emplace_back("row size");
        return row_size;
    }
    void InitializeRecord(pal4::GameDataMonsterRecord& record) override {
        record.fill(0);
        calls.emplace_back("init record");
    }
    void DestroyTemporaryRecordTexts(
        pal4::GameDataMonsterRecord&) override {
        calls.emplace_back("destroy texts");
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x6000 && index < bound_count,
              "append reads matching row property");
        calls.push_back("property " + std::to_string(index));
        return 0xa000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataMonsterRecord& record,
                      std::uint32_t property) override {
        const auto index = conversion_count++;
        Check(field == 0x7100 + index * 0x100 &&
                  property == 0xa000 + index,
              "bound field converts matching row property");
        record[index] = property;
        calls.push_back("convert " + std::to_string(index));
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataMonsterRecord& record) override {
        Check(array == 0x1000 + 92 && end == 0x8000 &&
                  (bound_count == 0 ||
                   (record[0] == 0xa000 && record[1] == 0xa001)),
              "append receives 596-byte converted record and array end");
        calls.emplace_back("append record");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x6000,
              "append error dump addresses");
        calls.emplace_back("dump");
        return " [row 8]";
    }
};

AppendRuntime MakeAppendRuntime() {
    AppendRuntime runtime;
    runtime.words[0x1000 + 128] = 0x7000;
    runtime.words[0x7000] = 0x7100;
    runtime.words[0x7004] = 0x7200;
    runtime.words[0x1000 + 100] = 0x8000;
    return runtime;
}

void CheckAppend() {
    auto runtime = MakeAppendRuntime();
    pal4::GameDataMonsterTable::Append(0x1000, 0x6000, runtime);
    Check(runtime.calls.front() == "init record" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "convert 0") != runtime.calls.end() &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "convert 1") != runtime.calls.end() &&
              runtime.calls[runtime.calls.size() - 2] == "append record" &&
              runtime.calls.back() == "destroy texts",
          "two fields convert in order before append and cleanup");

    auto empty = MakeAppendRuntime();
    empty.row_size = 0;
    empty.bound_count = 0;
    pal4::GameDataMonsterTable::Append(0x1000, 0x6000, empty);
    Check(empty.calls[empty.calls.size() - 2] == "append record" &&
              empty.calls.back() == "destroy texts",
          "empty row still appends an initialized record");

    auto failed = MakeAppendRuntime();
    failed.row_size = 3;
    bool threw = false;
    try {
        pal4::GameDataMonsterTable::Append(0x1000, 0x6000, failed);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) ==
                "ERROR <dbRecordBindTable< Record >::addRow > [row 8]";
    }
    Check(threw && failed.calls[failed.calls.size() - 2] == "dump" &&
              failed.calls.back() == "destroy texts",
          "row-size mismatch dumps and cleans temporary record");
}

void CheckMarkAndRefresh() {
    struct RefreshRuntime final : pal4::IGameDataMonsterRefreshRuntime {
        std::vector<std::string> calls;
        void WriteByte(std::uint32_t address,
                       std::uint8_t value) override {
            Check(address == 0x1000 + 88 && value == 1,
                  "dirty flag address and value");
            calls.emplace_back("flag");
        }
        std::uint32_t Refresh(std::uint32_t table) override {
            Check(table == 0x1000, "refresh receives table");
            calls.emplace_back("refresh");
            return 0xface;
        }
    } runtime;
    Check(pal4::GameDataMonsterTable::MarkAndRefresh(0x1000, runtime) ==
              0xface &&
              runtime.calls ==
                  std::vector<std::string>{"flag", "refresh"},
          "flag is written before refresh and result is forwarded");
}

void CheckRefreshBinding() {
    struct BindingRuntime final : pal4::IGameDataMonsterBindingRuntime {
        std::map<std::uint32_t, std::uint32_t> words;
        std::vector<std::string> calls;
        std::uint32_t ReadWord(std::uint32_t address) override {
            return words[address];
        }
        void LogBind(std::string_view prefix,
                     std::uint32_t table_name) override {
            Check(table_name == 0x1000 + 52,
                  "binding log reads table name at +52");
            calls.push_back(std::string(prefix));
        }
        void AssignEffects(std::uint32_t destination,
                           std::uint32_t source) override {
            Check(destination == 0x1000 + 108 && source == 0x1000 + 20,
                  "binding assignment uses +108 and +20");
            calls.emplace_back("assign");
        }
        void InitializeStructures(std::uint32_t binding) override {
            Check(binding == 0x1000 + 108,
                  "binding initialization uses +108");
            calls.emplace_back("initialize");
        }
        void BindEffects(std::uint32_t binding) override {
            Check(binding == 0x1000 + 108,
                  "Effect binding uses +108");
            calls.emplace_back("bind");
        }
    } runtime;

    pal4::GameDataMonsterTable::RefreshBinding(0x1000, runtime);
    Check(runtime.calls.empty(), "both empty collections need no rebinding");
    runtime.words[0x1000 + 128] = 0x3000;
    runtime.words[0x1000 + 132] = 0x3008;
    runtime.words[0x1000 + 24] = 0x4000;
    runtime.words[0x1000 + 28] = 0x4000 + 2 * 60;
    pal4::GameDataMonsterTable::RefreshBinding(0x1000, runtime);
    Check(runtime.calls.empty(), "equal nonzero counts need no rebinding");

    runtime.words[0x1000 + 28] = 0x4000 + 60;
    pal4::GameDataMonsterTable::RefreshBinding(0x1000, runtime);
    const std::vector<std::string> expected{
        "beginBindField, table:", "assign", "initialize", "bind",
        "endBindField, table:"};
    Check(runtime.calls == expected,
          "mismatched counts rebind and log in exact order");

    runtime.calls.clear();
    runtime.words[0x1000 + 128] = 0;
    pal4::GameDataMonsterTable::RefreshBinding(0x1000, runtime);
    Check(runtime.calls == expected,
          "field-only collection also triggers rebinding");
    runtime.calls.clear();
    runtime.words[0x1000 + 24] = 0;
    runtime.words[0x1000 + 128] = 0x3000;
    pal4::GameDataMonsterTable::RefreshBinding(0x1000, runtime);
    Check(runtime.calls == expected,
          "pointer-only collection also triggers rebinding");
}

void CheckFindRecord() {
    Runtime runtime;
    runtime.words[0x1004] = 0x2000;
    runtime.words[0x2008] = 0x3000;
    runtime.words[0x200c] = 0x3018;
    runtime.words[0x2060] = 0x5000;
    runtime.words[0x3000] = static_cast<std::uint32_t>(-5);
    runtime.words[0x3004] = 2;
    runtime.words[0x3008] = 3;
    runtime.words[0x300c] = 0;
    runtime.words[0x3010] = 9;
    runtime.words[0x3014] = 1;

    Check(pal4::GameDataMonsterTable::FindRecord(0x1000, -5, runtime) ==
              0x5000 + 2 * 596 &&
              pal4::GameDataMonsterTable::FindRecord(0x1000, 3, runtime) ==
                  0x5000 &&
              pal4::GameDataMonsterTable::FindRecord(0x1000, 9, runtime) ==
                  0x5000 + 596,
          "signed key lookup uses index and 596-byte record stride");
    Check(pal4::GameDataMonsterTable::FindRecord(0x1000, -6, runtime) == 0 &&
              pal4::GameDataMonsterTable::FindRecord(0x1000, 4, runtime) == 0,
          "missing keys return null");
    runtime.words[0x200c] = 0x3000;
    Check(pal4::GameDataMonsterTable::FindRecord(0x1000, 3, runtime) == 0,
          "empty index returns null");
}

void CheckPreloadAndVerify() {
    Runtime runtime;
    runtime.words[0x1000] = 0x8000;
    Check(pal4::GameDataMonsterTable::Preload(0x1000, runtime) == 0x72 &&
              runtime.calls.back() == "preload",
          "preload result byte is forwarded");
    Check(pal4::GameDataMonsterTable::Verify(0x1000, runtime) == 1 &&
              runtime.words[0x1004] == 0x2000 &&
              runtime.calls.back() == "write 4100",
          "verify binds the found table at manager plus four");

    Runtime missing;
    missing.words[0x1000] = 0x8000;
    missing.found_table = 0;
    bool threw = false;
    try {
        (void)pal4::GameDataMonsterTable::Verify(0x1000, missing);
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) ==
                "ERROR [ palDBMonsterManager::verifyDataTable < the table "
                "Monster  doesn't exist>]";
    }
    Check(threw && missing.words[0x1004] == 0 &&
              missing.calls[missing.calls.size() - 2] == "write 4100" &&
              missing.calls.back() == "log",
          "missing table is stored, logged and thrown in that order");
}

}  // namespace

int main() {
    try {
        CheckFindRecord();
        CheckPreloadAndVerify();
        CheckAddOrReplace();
        CheckAppend();
        CheckMarkAndRefresh();
        CheckRefreshBinding();
        std::cout << "game_data_monster_table_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
