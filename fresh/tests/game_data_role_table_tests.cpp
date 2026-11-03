#include "pal4/game_data_role_table.h"

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

struct Runtime final : pal4::IGameDataRoleRowRuntime,
                       pal4::IGameDataRoleAppendRuntime,
                       pal4::IGameDataRoleRefreshRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    pal4::GameDataRoleTableRecord last_record{};
    std::uint32_t position = 0;
    std::uint32_t collection_size = 0;
    std::uint32_t row_size = 0;
    std::uint32_t role_id = 1001;
    std::uint32_t category = 6;
    bool parsed = true;
    std::uint8_t flag = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x2000 + 124, "bound collection address");
        calls.emplace_back("size");
        return collection_size;
    }
    void LogBind(std::string_view prefix, std::uint32_t name) override {
        Check(name == 0x2000 + 52, "table name offset");
        calls.push_back("log:" + std::string(prefix));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x2000 + 108 && source == 0x2000 + 20,
              "effect bind offsets");
        calls.emplace_back("assign");
    }
    void InitializeStructures(std::uint32_t binding) override {
        Check(binding == 0x2000 + 108, "registry binding offset");
        calls.emplace_back("structures");
    }
    void BindEffects(std::uint32_t binding) override {
        Check(binding == 0x2000 + 108, "effect binding offset");
        calls.emplace_back("bind");
    }
    void InitializeRecord(pal4::GameDataRoleTableRecord& record) override {
        calls.emplace_back("initialize");
        record[0] = category;
        record[57] = role_id;
    }
    void DestroyTemporaryRecord(
        pal4::GameDataRoleTableRecord& record) override {
        last_record = record;
        calls.emplace_back("destroy temporary");
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::GameDataRoleTableRecord& record) override {
        Check(binding == 0x2000 + 108 && row == 0x3000,
              "parser arguments");
        calls.emplace_back("parse");
        record[0] = category;
        record[57] = role_id;
        return parsed;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t key) override {
        Check(begin == words[0x2008] && end == words[0x200c] &&
                  key == role_id, "lower-bound arguments");
        calls.emplace_back("find");
        return position;
    }
    void CopyRecord(std::uint32_t destination,
                    const pal4::GameDataRoleTableRecord& record) override {
        Check(destination == 0x5000 + 640,
              "replacement address uses 320-byte stride");
        last_record = record;
        calls.emplace_back("copy");
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataRoleTableRecord& record) override {
        Check(array == 0x2000 + 92 && end == words[0x2064],
              "record insert array and end");
        last_record = record;
        calls.emplace_back("insert record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x2000 + 4 && at == position &&
                  key == role_id && index == 2,
              "key insertion stores original record index");
        calls.emplace_back("insert key");
    }
    std::string DumpRow(std::uint32_t table, std::uint32_t row) override {
        Check(table == 0x2000 && row == 0x3000,
              "error dump arguments");
        calls.emplace_back("dump");
        return "ROW";
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x3000, "append row address");
        calls.emplace_back("row size");
        return row_size;
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x3000 && index < 2, "property lookup");
        calls.emplace_back("property");
        return 0x7000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataRoleTableRecord& record,
                      std::uint32_t property) override {
        const auto index = property - 0x7000;
        Check(field == 0x8000 + index && index < 2,
              "bound field and property order");
        record[10 + index] = property;
        calls.emplace_back("convert");
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataRoleTableRecord& record) override {
        Check(array == 0x2000 + 92 && end == words[0x2064],
              "append array and end");
        last_record = record;
        calls.emplace_back("append");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x2000 + 88, "dirty byte address");
        flag = value;
        calls.emplace_back("dirty");
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x2000, "refresh table address");
        calls.emplace_back("refresh");
        return 0x55;
    }
};

Runtime Base() {
    Runtime runtime;
    runtime.words[0x2000 + 24] = 0x4000;
    runtime.words[0x2000 + 28] = 0x4000 + 120;
    runtime.words[0x2000 + 8] = 0x6000;
    runtime.words[0x2000 + 12] = 0x6010;
    runtime.words[0x2000 + 96] = 0x5000;
    runtime.words[0x2000 + 100] = 0x5000 + 640;
    runtime.words[0x2000 + 128] = 0x9000;
    runtime.words[0x9000] = 0x8000;
    runtime.words[0x9004] = 0x8001;
    runtime.collection_size = 2;
    runtime.row_size = 2;
    runtime.position = 0x6010;
    return runtime;
}

void CheckAddOrReplace() {
    auto insert = Base();
    Check(pal4::GameDataRoleTable::AddOrReplace(0x2000, 0x3000,
                                                insert) == 1 &&
              insert.last_record[56] == 0 &&
              insert.last_record[57] == 1001,
          "insert normalizes Role ID before storing record");
    const auto find = [&insert](std::string_view name) {
        return std::find(insert.calls.begin(), insert.calls.end(), name) -
               insert.calls.begin();
    };
    Check(find("parse") < find("find") &&
              find("find") < find("insert record") &&
              find("insert record") < find("insert key") &&
              find("insert key") < find("destroy temporary"),
          "insert order and temporary cleanup");
    Check(find("assign") == insert.calls.size(),
          "matching field count skips bind");

    auto replace = Base();
    replace.position = 0x6000;
    replace.words[0x6000] = 1001;
    replace.words[0x6004] = 2;
    replace.role_id = 1001;
    Check(pal4::GameDataRoleTable::AddOrReplace(0x2000, 0x3000,
                                                replace) == 1 &&
              std::find(replace.calls.begin(), replace.calls.end(),
                        "copy") != replace.calls.end() &&
              std::find(replace.calls.begin(), replace.calls.end(),
                        "insert record") == replace.calls.end(),
          "existing signed key replaces record at indexed slot");

    auto rebind = Base();
    rebind.collection_size = 1;
    static_cast<void>(pal4::GameDataRoleTable::AddOrReplace(
        0x2000, 0x3000, rebind));
    Check(std::find(rebind.calls.begin(), rebind.calls.end(),
                    "log:beginBindField, table:") <
                  std::find(rebind.calls.begin(), rebind.calls.end(),
                            "assign") &&
              std::find(rebind.calls.begin(), rebind.calls.end(),
                        "log:endBindField, table:") <
                  std::find(rebind.calls.begin(), rebind.calls.end(),
                            "initialize"),
          "field count mismatch rebinds before parsing");
}

void CheckFailures() {
    for (std::uint32_t id : {1000u, 1005u}) {
        auto runtime = Base();
        runtime.role_id = id;
        bool threw = false;
        try {
            static_cast<void>(pal4::GameDataRoleTable::AddOrReplace(
                0x2000, 0x3000, runtime));
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()) ==
                    "ERROR <dbRecordBindTable< Record >::addRow >ROW";
        }
        Check(threw && runtime.last_record[56] == id - 1001u &&
                  runtime.calls.back() == "destroy temporary",
              "invalid ID records normalized and cleaned after error");
    }
    auto category = Base();
    category.category = 7;
    bool threw = false;
    try {
        static_cast<void>(pal4::GameDataRoleTable::AddOrReplace(
            0x2000, 0x3000, category));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && category.last_record[56] == 0,
          "category above six rejected after normalization");
    auto parse = Base();
    parse.parsed = false;
    threw = false;
    try {
        static_cast<void>(pal4::GameDataRoleTable::AddOrReplace(
            0x2000, 0x3000, parse));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && parse.last_record[56] == 0 &&
              std::find(parse.calls.begin(), parse.calls.end(),
                        "find") == parse.calls.end(),
          "parse failure skips validation and index search");
}

void CheckAppendAndRefresh() {
    auto runtime = Base();
    pal4::GameDataRoleTable::Append(0x2000, 0x3000, runtime);
    Check(runtime.last_record[10] == 0x7000 &&
              runtime.last_record[11] == 0x7001 &&
              runtime.last_record[56] == 0 &&
              runtime.calls[runtime.calls.size() - 2] == "append" &&
              runtime.calls.back() == "destroy temporary",
          "append converts fields and releases temporary texts");
    auto bad_count = Base();
    bad_count.row_size = 1;
    bool threw = false;
    try {
        pal4::GameDataRoleTable::Append(0x2000, 0x3000, bad_count);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && bad_count.calls.back() == "destroy temporary" &&
              std::find(bad_count.calls.begin(), bad_count.calls.end(),
                        "property") == bad_count.calls.end(),
          "row length mismatch skips conversion and cleans temporary");
    auto bad_role = Base();
    bad_role.role_id = 1005;
    threw = false;
    try {
        pal4::GameDataRoleTable::Append(0x2000, 0x3000, bad_role);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && bad_role.last_record[56] == 4 &&
              std::find(bad_role.calls.begin(), bad_role.calls.end(),
                        "append") == bad_role.calls.end(),
          "invalid converted record rejected before append");
    auto refresh = Base();
    Check(pal4::GameDataRoleTable::MarkAndRefresh(0x2000, refresh) ==
              0x55 && refresh.flag == 1 &&
              refresh.calls ==
                  std::vector<std::string>{"dirty", "refresh"},
          "mark dirty precedes refresh and forwards result");
}

}  // namespace

int main() {
    try {
        CheckAddOrReplace();
        CheckFailures();
        CheckAppendAndRefresh();
        std::cout << "game_data_role_table_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
