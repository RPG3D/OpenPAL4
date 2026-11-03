#include "pal4/game_data_equip_relation_table.h"

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

struct Runtime final : pal4::IGameDataEquipRelationTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1018, 0x2000}, {0x101c, 0x2078}, {0x1080, 0x3000},
        {0x3000, 0x4000}, {0x3004, 0x4100}, {0x1008, 0x5000},
        {0x100c, 0x5010}, {0x5000, 0xffffffff}, {0x5004, 1},
        {0x1060, 0x6000}, {0x1064, 0x6024}};
    std::vector<std::string> calls;
    std::uint32_t collection_size = 2;
    std::uint32_t row_size = 2;
    std::uint32_t found = 0x5000;
    std::uint32_t converted_key = 0xffffffff;
    std::uint32_t append_result = 0x7777;
    bool members_valid = true;
    pal4::GameDataEquipRelationRecord copied{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t collection) override {
        Check(collection == 0x107c, "collection offset");
        return collection_size;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x9000, "row address");
        calls.emplace_back("row size");
        return row_size;
    }
    void LogBind(std::string_view prefix, std::uint32_t table_name) override {
        Check(table_name == 0x1034, "table name offset");
        calls.emplace_back(prefix);
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x106c && source == 0x1014,
              "bind assignment offsets");
        calls.emplace_back("assign");
    }
    void InitializeStructures(std::uint32_t object) override {
        Check(object == 0x106c, "structure offset");
        calls.emplace_back("initialize");
    }
    void BindEffects(std::uint32_t object) override {
        Check(object == 0x106c, "bind offset");
        calls.emplace_back("bind");
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x9000 && index < collection_size,
              "property index");
        calls.push_back("property " + std::to_string(index));
        return 0xa000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataEquipRelationRecord& record,
                      std::uint32_t property) override {
        Check(field == 0x4000 + 0x100 * (property - 0xa000),
              "field and property pair");
        calls.push_back("convert " + std::to_string(property - 0xa000));
        record[property - 0xa000 + 1] = property;
        record[0] = converted_key;
    }
    bool CheckMembersRange(
        const pal4::GameDataEquipRelationRecord& record) override {
        Check(record[0] == converted_key, "range receives converted row");
        calls.emplace_back("check range");
        return members_valid;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t key) override {
        Check(begin == 0x5000 && end == 0x5010 && key == converted_key,
              "search range and signed key bits");
        calls.emplace_back("find");
        return found;
    }
    void CopyRecord(std::uint32_t destination,
                    const pal4::GameDataEquipRelationRecord& record) override {
        Check(destination == 0x600c, "indexed 12-byte record address");
        copied = record;
        calls.emplace_back("copy");
    }
    void InsertRecord(std::uint32_t array, std::uint32_t position,
                      const pal4::GameDataEquipRelationRecord& record) override {
        Check(array == 0x105c && position == 0x6024 &&
                  record[0] == converted_key,
              "record insertion arguments");
        calls.emplace_back("insert record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t position,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && position == found &&
                  key == converted_key && index == 3,
              "key insertion follows three existing records");
        calls.emplace_back("insert key");
    }
    std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const pal4::GameDataEquipRelationRecord& record) override {
        Check(array == 0x105c && end == 0x6024 &&
                  record == pal4::GameDataEquipRelationRecord{
                                converted_key, 0xa000, 0xa001},
              "append receives complete 12-byte record");
        calls.emplace_back("append");
        return append_result;
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x9000, "dump row context");
        calls.emplace_back("dump");
        return ": relation row 17";
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x1058 && value == 1, "dirty byte offset");
        calls.emplace_back("flag");
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x1000, "refresh table pointer");
        calls.emplace_back("refresh");
        return 0x12345678;
    }
};

void CheckAddOrReplace() {
    Runtime replacement;
    Check(pal4::GameDataEquipRelationTable::AddOrReplace(
              0x1000, 0x9000, replacement) == 1 &&
              replacement.copied ==
                  pal4::GameDataEquipRelationRecord{
                      0xffffffff, 0xa000, 0xa001} &&
              replacement.calls.back() == "copy",
          "existing signed key replaces its indexed relation record");

    Runtime insertion;
    insertion.found = 0x5010;
    Check(pal4::GameDataEquipRelationTable::AddOrReplace(
              0x1000, 0x9000, insertion) == 1 &&
              insertion.calls[insertion.calls.size() - 2] == "insert record" &&
              insertion.calls.back() == "insert key",
          "missing key appends relation before key index");

    Runtime signed_order;
    signed_order.converted_key = 0x80000000;
    Check(pal4::GameDataEquipRelationTable::AddOrReplace(
              0x1000, 0x9000, signed_order) == 1 &&
              signed_order.calls.back() == "insert key",
          "negative signed key before minus one inserts instead of replacing");

    Runtime rebinding;
    rebinding.words[0x101c] = 0x203c;
    Check(pal4::GameDataEquipRelationTable::AddOrReplace(
              0x1000, 0x9000, rebinding) == 1 &&
              std::vector<std::string>(rebinding.calls.begin(),
                                       rebinding.calls.begin() + 5) ==
                  std::vector<std::string>{
                      "beginBindField, table:", "assign", "initialize",
                      "bind", "endBindField, table:"},
          "field mismatch rebinds and logs in IDA order");
}

void CheckAppendAndErrors() {
    Runtime append;
    Check(pal4::GameDataEquipRelationTable::Append(
              0x1000, 0x9000, append) == 0x7777 &&
              append.calls.back() == "append",
          "append returns the native insertion result");

    for (bool bad_size : {true, false}) {
        Runtime invalid;
        if (bad_size) invalid.row_size = 1;
        else invalid.members_valid = false;
        bool thrown = false;
        try {
            (void)pal4::GameDataEquipRelationTable::AddOrReplace(
                0x1000, 0x9000, invalid);
        } catch (const std::runtime_error& error) {
            thrown = std::string_view(error.what()) ==
                     "ERROR <dbRecordBindTable< Record >::addRow >: relation row 17";
        }
        Check(thrown && invalid.calls.back() == "dump" &&
                  (bad_size ||
                   std::find(invalid.calls.begin(), invalid.calls.end(),
                             "check range") != invalid.calls.end()),
              "invalid field count or member range dumps row before throw");
    }

    Runtime invalid_append;
    invalid_append.members_valid = false;
    bool thrown = false;
    try {
        (void)pal4::GameDataEquipRelationTable::Append(
            0x1000, 0x9000, invalid_append);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && invalid_append.calls.back() == "dump",
          "append rejects invalid member range");
}

void CheckRefresh() {
    Runtime runtime;
    Check(pal4::GameDataEquipRelationTable::MarkAndRefresh(
              0x1000, runtime) == 0x12345678 &&
              runtime.calls == std::vector<std::string>{"flag", "refresh"},
          "dirty flag precedes refresh and forwards result");
}

}  // namespace

int main() {
    try {
        CheckAddOrReplace();
        CheckAppendAndErrors();
        CheckRefresh();
        std::cout << "game_data_equip_relation_table_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
