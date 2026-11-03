#include "pal4/game_data_ai_record_table.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataAiRecordTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t collection_size = 2;
    std::uint32_t row_size = 2;
    std::uint32_t found = 0x5000;
    std::uint32_t append_result = 0x7777;
    bool valid = true;
    pal4::GameDataAiRecord copied{};

    Runtime() {
        words[0x1018] = 0x2000;
        words[0x101c] = 0x2078;
        words[0x1080] = 0x3000;
        words[0x1084] = 0x3008;
        words[0x3000] = 0x4000;
        words[0x3004] = 0x4100;
        words[0x1008] = 0x5000;
        words[0x100c] = 0x5010;
        words[0x5000] = 12;
        words[0x5004] = 1;
        words[0x1060] = 0x6000;
        words[0x1064] = 0x6070;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x107c, "collection address");
        return collection_size;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x9000, "row address");
        calls.emplace_back("row size");
        return row_size;
    }
    void LogBind(std::string_view prefix, std::uint32_t name) override {
        Check(name == 0x1034, "table name address");
        calls.push_back(std::string(prefix));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x106c && source == 0x1014,
              "effect assignment addresses");
        calls.emplace_back("assign");
    }
    void InitializeStructures(std::uint32_t object) override {
        Check(object == 0x106c, "initialization address");
        calls.emplace_back("initialize");
    }
    void BindEffects(std::uint32_t object) override {
        Check(object == 0x106c, "bind address");
        calls.emplace_back("bind");
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x9000 && index < 2, "property index");
        calls.push_back("property " + std::to_string(index));
        return 0xa000 + index;
    }
    void ConvertField(std::uint32_t field, pal4::GameDataAiRecord& record,
                      std::uint32_t property) override {
        Check((field == 0x4000 && property == 0xa000) ||
                  (field == 0x4100 && property == 0xa001),
              "bound field and row property pair");
        calls.push_back("convert " + std::to_string(field));
        record[0] = 12;
        record[1] = property;
        record[13] = 0x4321;
    }
    bool RecordValid(const pal4::GameDataAiRecord& record) override {
        Check(record[0] == 12 && record[13] == 0x4321,
              "last conversion result is checked");
        calls.emplace_back("validate");
        return valid;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t key) override {
        Check(begin == 0x5000 && end == 0x5010 && key == 12,
              "search range and key");
        calls.emplace_back("find");
        return found;
    }
    void CopyRecord(std::uint32_t destination,
                    const pal4::GameDataAiRecord& record) override {
        Check(destination == 0x6038, "indexed 56-byte record destination");
        copied = record;
        calls.emplace_back("copy");
    }
    void InsertRecord(std::uint32_t array, std::uint32_t position,
                      const pal4::GameDataAiRecord& record) override {
        Check(array == 0x105c && position == 0x6070 && record[0] == 12,
              "new record appends at old end");
        calls.emplace_back("insert record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t position,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && position == found && key == 12 && index == 2,
              "new sorted key stores the previous record count");
        calls.emplace_back("insert key");
    }
    std::uint32_t AppendRecord(std::uint32_t array, std::uint32_t end,
                               const pal4::GameDataAiRecord& record) override {
        Check(array == 0x105c && end == 0x6070 && record[13] == 0x4321,
              "append forwards the whole converted record");
        calls.emplace_back("append");
        return append_result;
    }
    std::string DumpRow(std::uint32_t table, std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x9000, "error includes row context");
        calls.emplace_back("dump");
        return ": row 17";
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x1058, "state flag address");
        calls.push_back("flag " + std::to_string(value));
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x1000, "refresh owner");
        calls.emplace_back("refresh");
        return 0x1234;
    }
    std::uint32_t DispatchVirtual24(std::uint32_t table) override {
        Check(table == 0x1000, "virtual call owner");
        calls.emplace_back("virtual 24");
        return 0x5678;
    }
};

std::size_t At(const Runtime& runtime, const std::string& value) {
    auto found = std::find(runtime.calls.begin(), runtime.calls.end(), value);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckAddOrReplace() {
    Runtime runtime;
    Check(pal4::GameDataAiRecordTable::AddOrReplace(0x1000, 0x9000,
                                                    runtime) == 1 &&
              runtime.calls.front() == "row size" &&
              At(runtime, "convert 16384") < At(runtime, "convert 16640") &&
              At(runtime, "validate") < At(runtime, "find") &&
              runtime.calls.back() == "copy" &&
              runtime.copied[1] == 0xa001,
          "matching key replaces the indexed record after conversion");

    Runtime insertion;
    insertion.found = 0x5010;
    Check(pal4::GameDataAiRecordTable::AddOrReplace(0x1000, 0x9000,
                                                    insertion) == 1 &&
              At(insertion, "insert record") < At(insertion, "insert key") &&
              insertion.calls.back() == "insert key",
          "missing key appends value and inserts sorted key");

    Runtime rebinding;
    rebinding.words[0x101c] = 0x203c;
    rebinding.collection_size = 2;
    Check(pal4::GameDataAiRecordTable::AddOrReplace(0x1000, 0x9000,
                                                    rebinding) == 1 &&
              rebinding.calls[0] == "beginBindField, table:" &&
              rebinding.calls[1] == "assign" &&
              rebinding.calls[2] == "initialize" &&
              rebinding.calls[3] == "bind" &&
              rebinding.calls[4] == "endBindField, table:" &&
              rebinding.calls[5] == "row size",
          "field count change rebinds before row conversion");
}

void CheckErrors() {
    for (bool invalid_record : {false, true}) {
        Runtime runtime;
        if (invalid_record) runtime.valid = false;
        else runtime.row_size = 1;
        try {
            (void)pal4::GameDataAiRecordTable::AddOrReplace(
                0x1000, 0x9000, runtime);
            Check(false, "invalid row should throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      "ERROR <dbRecordBindTable< Record >::addRow >: row 17" &&
                      runtime.calls.back() == "dump" &&
                      std::find(runtime.calls.begin(), runtime.calls.end(),
                                "find") == runtime.calls.end(),
                  "invalid row throws before map mutation");
        }
    }
}

void CheckAppend() {
    Runtime runtime;
    Check(pal4::GameDataAiRecordTable::Append(0x1000, 0x9000, runtime) ==
              0x7777 && runtime.calls.back() == "append" &&
              At(runtime, "validate") < At(runtime, "append"),
          "append forwards the insertion result");

    Runtime invalid;
    invalid.row_size = 1;
    try {
        (void)pal4::GameDataAiRecordTable::Append(0x1000, 0x9000, invalid);
        Check(false, "append size mismatch should throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()).find("addRow") != std::string::npos &&
                  invalid.calls.back() == "dump",
              "append size mismatch includes row details");
    }
}

void CheckStateSwitches() {
    Runtime runtime;
    Check(pal4::GameDataAiRecordTable::MarkAndRefresh(0x1000, runtime) ==
              0x1234 &&
              runtime.calls == std::vector<std::string>{"flag 1", "refresh"},
          "mark precedes refresh and forwards its return");
    runtime.calls.clear();
    Check(pal4::GameDataAiRecordTable::ClearAndDispatch(0x1000, runtime) ==
              0x5678 &&
              runtime.calls ==
                  std::vector<std::string>{"flag 0", "virtual 24"},
          "clear precedes vtable dispatch and forwards its return");
}

}  // namespace

int main() {
    try {
        CheckAddOrReplace();
        CheckErrors();
        CheckAppend();
        CheckStateSwitches();
        std::cout << "game_data_ai_record_table_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
