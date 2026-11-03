#include "pal4/game_data_upgrade_table.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataUpgradeRowRuntime,
                       pal4::IGameDataUpgradeAppendRuntime,
                       pal4::IGameDataUpgradeRefreshRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    pal4::GameDataUpgradeTableRecord last_record{};
    std::uint32_t collection_size = 2;
    std::uint32_t row_size = 2;
    std::uint32_t key = 100;
    std::uint32_t role_id = 1001;
    std::uint32_t position = 0x6010;
    std::uint8_t dirty = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x207c, "bound collection offset");
        calls.emplace_back("collection size");
        return collection_size;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x3000, "row size address");
        calls.emplace_back("row size");
        return row_size;
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t name) override {
        Check(name == 0x2034, "table name address");
        calls.push_back("log:" + std::string(prefix));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x206c && source == 0x2014,
              "Effect binding offsets");
        calls.emplace_back("assign");
    }
    void InitializeStructures(std::uint32_t binding) override {
        Check(binding == 0x206c, "attribute registry binding");
        calls.emplace_back("structures");
    }
    void BindEffects(std::uint32_t binding) override {
        Check(binding == 0x206c, "Effect bind address");
        calls.emplace_back("bind");
    }
    void InitializeRecord(
        pal4::GameDataUpgradeTableRecord& record) override {
        record.fill(0);
        record[0] = 7;
        record[36] = key;
        record[37] = role_id;
        calls.emplace_back("initialize");
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x3000 && index < 2, "property address");
        calls.push_back("property:" + std::to_string(index));
        return 0x7000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataUpgradeTableRecord& record,
                      std::uint32_t property) override {
        const auto index = property - 0x7000;
        Check(field == 0x8000 + index && index < 2,
              "bound field and property pairing");
        record[10 + index] = property;
        calls.push_back("convert:" + std::to_string(index));
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t requested) override {
        Check(begin == 0x6000 && end == 0x6010 && requested == key,
              "signed lower-bound input");
        calls.emplace_back("find");
        return position;
    }
    void CopyRecord(std::uint32_t destination,
                    const pal4::GameDataUpgradeTableRecord& record) override {
        Check(destination == 0x5000 + 160,
              "replacement uses 160-byte record stride");
        last_record = record;
        calls.emplace_back("copy");
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataUpgradeTableRecord& record) override {
        Check(array == 0x205c && end == 0x5000 + 320,
              "record array insert position");
        last_record = record;
        calls.emplace_back("insert record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t requested, std::uint32_t index) override {
        Check(array == 0x2004 && at == position &&
                  requested == key && index == 2,
              "new key stores old record count");
        calls.emplace_back("insert key");
    }
    std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const pal4::GameDataUpgradeTableRecord& record) override {
        Check(array == 0x205c && end == 0x5000 + 320,
              "append address and end");
        last_record = record;
        calls.emplace_back("append");
        return 0x77;
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        Check(table == 0x2000 && row == 0x3000,
              "error row dump arguments");
        calls.emplace_back("dump");
        return "ROW";
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x2058, "dirty flag offset");
        dirty = value;
        calls.emplace_back("dirty");
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x2000, "refresh table pointer");
        calls.emplace_back("refresh");
        return 0x55;
    }
};

Runtime Base() {
    Runtime runtime;
    runtime.words[0x2018] = 0x4000;
    runtime.words[0x201c] = 0x4000 + 120;
    runtime.words[0x2080] = 0x9000;
    runtime.words[0x2084] = 0x9000 + 8;
    runtime.words[0x9000] = 0x8000;
    runtime.words[0x9004] = 0x8001;
    runtime.words[0x2008] = 0x6000;
    runtime.words[0x200c] = 0x6010;
    runtime.words[0x2060] = 0x5000;
    runtime.words[0x2064] = 0x5000 + 320;
    return runtime;
}

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void CheckAddOrReplace() {
    auto insert = Base();
    Check(pal4::GameDataUpgradeTable::AddOrReplace(
              0x2000, 0x3000, insert) == 1 &&
              insert.last_record[0] == 7 &&
              insert.last_record[10] == 0x7000 &&
              insert.last_record[11] == 0x7001 &&
              insert.last_record[36] == 100 &&
              insert.last_record[37] == 1001,
          "converted 160-byte record contains initialized fields and key");
    Check(At(insert, "initialize") < At(insert, "row size") &&
              At(insert, "convert:0") < At(insert, "convert:1") &&
              At(insert, "find") < At(insert, "insert record") &&
              At(insert, "insert record") < At(insert, "insert key") &&
              std::find(insert.calls.begin(), insert.calls.end(),
                        "assign") == insert.calls.end(),
          "record insert follows validation and skips matching bind");

    auto replace = Base();
    replace.position = 0x6000;
    replace.words[0x6000] = 100;
    replace.words[0x6004] = 1;
    Check(pal4::GameDataUpgradeTable::AddOrReplace(
              0x2000, 0x3000, replace) == 1 &&
              At(replace, "copy") < replace.calls.size() &&
              std::find(replace.calls.begin(), replace.calls.end(),
                        "insert record") == replace.calls.end(),
          "existing key replaces indexed record by 160-byte copy");

    auto rebind = Base();
    rebind.collection_size = 1;
    rebind.row_size = 1;
    static_cast<void>(pal4::GameDataUpgradeTable::AddOrReplace(
        0x2000, 0x3000, rebind));
    Check(At(rebind, "log:beginBindField, table:") <
                  At(rebind, "assign") &&
              At(rebind, "assign") < At(rebind, "structures") &&
              At(rebind, "structures") < At(rebind, "bind") &&
              At(rebind, "log:endBindField, table:") <
                  At(rebind, "initialize"),
          "binding mismatch refreshes descriptors before row conversion");
}

void CheckFailures() {
    for (std::uint32_t id : {1000u, 1005u}) {
        auto runtime = Base();
        runtime.role_id = id;
        bool threw = false;
        try {
            static_cast<void>(pal4::GameDataUpgradeTable::AddOrReplace(
                0x2000, 0x3000, runtime));
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()) ==
                    "ERROR <dbRecordBindTable< Record >::addRow >ROW";
        }
        Check(threw && At(runtime, "dump") < runtime.calls.size() &&
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "find") == runtime.calls.end(),
              "invalid linked Role ID stops before index modification");
    }
    auto mismatch = Base();
    mismatch.row_size = 1;
    bool threw = false;
    try {
        static_cast<void>(pal4::GameDataUpgradeTable::AddOrReplace(
            0x2000, 0x3000, mismatch));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && std::find(mismatch.calls.begin(), mismatch.calls.end(),
                             "convert:0") == mismatch.calls.end(),
          "row size mismatch skips conversion");
}

void CheckAppendAndRefresh() {
    auto runtime = Base();
    Check(pal4::GameDataUpgradeTable::Append(0x2000, 0x3000, runtime) ==
              0x77 && runtime.last_record[36] == 100 &&
              runtime.last_record[37] == 1001 &&
              runtime.last_record[11] == 0x7001 &&
              runtime.calls.back() == "append",
          "append validates and forwards native return");
    auto count = Base();
    count.words[0x2084] = 0x9000 + 4;
    bool threw = false;
    try {
        static_cast<void>(pal4::GameDataUpgradeTable::Append(
            0x2000, 0x3000, count));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && std::find(count.calls.begin(), count.calls.end(),
                             "convert:0") == count.calls.end(),
          "append compares row size to bound pointer count");
    auto invalid = Base();
    invalid.role_id = 1005;
    threw = false;
    try {
        static_cast<void>(pal4::GameDataUpgradeTable::Append(
            0x2000, 0x3000, invalid));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && std::find(invalid.calls.begin(), invalid.calls.end(),
                             "append") == invalid.calls.end(),
          "append rejects invalid linked Role ID");
    auto refresh = Base();
    Check(pal4::GameDataUpgradeTable::MarkAndRefresh(
              0x2000, refresh) == 0x55 && refresh.dirty == 1 &&
              refresh.calls ==
                  std::vector<std::string>{"dirty", "refresh"},
          "dirty flag precedes refresh and result is forwarded");
}

}  // namespace

int main() {
    try {
        CheckAddOrReplace();
        CheckFailures();
        CheckAppendAndRefresh();
        std::cout << "game_data_upgrade_table_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
