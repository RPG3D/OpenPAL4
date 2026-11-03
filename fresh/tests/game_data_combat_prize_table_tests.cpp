#include "pal4/game_data_combat_prize_table.h"

#include <algorithm>
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

struct Runtime final : pal4::IGameDataCombatPrizeRowRuntime,
                       pal4::IGameDataCombatPrizeAppendRuntime,
                       pal4::IGameDataCombatPrizeRefreshRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    pal4::GameDataCombatPrizeRecord last{};
    std::uint32_t collection_size = 2;
    std::uint32_t row_size = 2;
    std::uint32_t key = 13;
    std::uint32_t position = 0x6010;
    std::uint8_t dirty = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x207c, "binding collection address");
        calls.emplace_back("count");
        return collection_size;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x3000, "row address");
        calls.emplace_back("row size");
        return row_size;
    }
    void LogBind(std::string_view prefix, std::uint32_t name) override {
        Check(name == 0x2034, "binding table name");
        calls.push_back(std::string(prefix));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x206c && source == 0x2014,
              "Effect assignment addresses");
        calls.emplace_back("assign");
    }
    void InitializeStructures(std::uint32_t address) override {
        Check(address == 0x206c, "structure initialization address");
        calls.emplace_back("structures");
    }
    void BindEffects(std::uint32_t address) override {
        Check(address == 0x206c, "binding address");
        calls.emplace_back("bind");
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x3000 && index < 2, "property lookup");
        calls.push_back("property:" + std::to_string(index));
        return 0x7000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataCombatPrizeRecord& record,
                      std::uint32_t property) override {
        const auto index = property - 0x7000;
        Check(index < 2 && field == 0x8000 + index,
              "field/property pair");
        record[index] = index == 0 ? key : property;
        calls.push_back("convert:" + std::to_string(index));
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t requested) override {
        Check(begin == 0x6000 && end == 0x6010 && requested == key,
              "signed key lookup arguments");
        calls.emplace_back("find");
        return position;
    }
    void CopyRecord(std::uint32_t destination,
                    const pal4::GameDataCombatPrizeRecord& record) override {
        Check(destination == 0x5080, "128-byte replacement stride");
        last = record;
        calls.emplace_back("copy");
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataCombatPrizeRecord& record) override {
        Check(array == 0x205c && end == 0x5100,
              "record insertion address");
        last = record;
        calls.emplace_back("insert record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t requested, std::uint32_t index) override {
        Check(array == 0x2004 && at == position &&
                  requested == key && index == 2,
              "key insertion stores old 128-byte record count");
        calls.emplace_back("insert key");
    }
    std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const pal4::GameDataCombatPrizeRecord& record) override {
        Check(array == 0x205c && end == 0x5100,
              "record append address");
        last = record;
        calls.emplace_back("append");
        return 0x55;
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
        Check(address == 0x2058, "dirty flag address");
        dirty = value;
        calls.emplace_back("dirty");
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x2000, "refresh table address");
        calls.emplace_back("refresh");
        return 0x77;
    }
};

Runtime Base() {
    Runtime runtime;
    runtime.words[0x2018] = 0x4000;
    runtime.words[0x201c] = 0x4000 + 120;
    runtime.words[0x2080] = 0x9000;
    runtime.words[0x9000] = 0x8000;
    runtime.words[0x9004] = 0x8001;
    runtime.words[0x2008] = 0x6000;
    runtime.words[0x200c] = 0x6010;
    runtime.words[0x2060] = 0x5000;
    runtime.words[0x2064] = 0x5100;
    return runtime;
}

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(),
                                 runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected operation missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckInsertReplaceAndBind() {
    auto insert = Base();
    Check(pal4::GameDataCombatPrizeTable::AddOrReplace(
              0x2000, 0x3000, insert) == 1 &&
              insert.last[0] == 13 && insert.last[1] == 0x7001 &&
              At(insert, "convert:0") < At(insert, "convert:1") &&
              At(insert, "insert record") < At(insert, "insert key"),
          "new key inserts converted record before key pair");

    auto replace = Base();
    replace.position = 0x6000;
    replace.words[0x6000] = 13;
    replace.words[0x6004] = 1;
    Check(pal4::GameDataCombatPrizeTable::AddOrReplace(
              0x2000, 0x3000, replace) == 1 &&
              At(replace, "copy") < replace.calls.size() &&
              std::find(replace.calls.begin(), replace.calls.end(),
                        "insert key") == replace.calls.end(),
          "existing key overwrites indexed 128-byte record");

    auto rebind = Base();
    rebind.collection_size = 1;
    rebind.row_size = 1;
    static_cast<void>(pal4::GameDataCombatPrizeTable::AddOrReplace(
        0x2000, 0x3000, rebind));
    Check(At(rebind, "beginBindField, table:") < At(rebind, "assign") &&
              At(rebind, "assign") < At(rebind, "structures") &&
              At(rebind, "structures") < At(rebind, "bind") &&
              At(rebind, "bind") < At(rebind, "endBindField, table:") &&
              At(rebind, "endBindField, table:") < At(rebind, "convert:0"),
          "mismatched binding is rebuilt before row conversion");
}

void CheckFailuresAppendAndRefresh() {
    auto invalid = Base();
    invalid.row_size = 1;
    bool threw = false;
    try {
        static_cast<void>(pal4::GameDataCombatPrizeTable::AddOrReplace(
            0x2000, 0x3000, invalid));
    } catch (const std::runtime_error& error) {
        threw = std::string(error.what()) ==
                "ERROR <dbRecordBindTable< Record >::addRow >ROW";
    }
    Check(threw && At(invalid, "dump") < invalid.calls.size() &&
              std::find(invalid.calls.begin(), invalid.calls.end(),
                        "convert:0") == invalid.calls.end(),
          "bad row length fails before conversion");

    auto append = Base();
    Check(pal4::GameDataCombatPrizeTable::Append(
              0x2000, 0x3000, append) == 0x55 &&
              append.last[0] == 13 && append.last[1] == 0x7001 &&
              append.calls.back() == "append",
          "append converts row and forwards native return");
    append = Base();
    append.row_size = 1;
    threw = false;
    try {
        static_cast<void>(pal4::GameDataCombatPrizeTable::Append(
            0x2000, 0x3000, append));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && std::find(append.calls.begin(), append.calls.end(),
                             "append") == append.calls.end(),
          "append rejects mismatched row length");

    auto refresh = Base();
    Check(pal4::GameDataCombatPrizeTable::MarkAndRefresh(
              0x2000, refresh) == 0x77 && refresh.dirty == 1 &&
              refresh.calls ==
                  std::vector<std::string>{"dirty", "refresh"},
          "dirty byte precedes refresh and result is forwarded");
}

}  // namespace

int main() {
    try {
        CheckInsertReplaceAndBind();
        CheckFailuresAppendAndRefresh();
        std::cout << "game_data_combat_prize_table_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
