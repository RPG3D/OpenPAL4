#include "pal4/game_data_equip_table.h"

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

struct Runtime final : pal4::IGameDataEquipTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1018, 0x2000}, {0x101c, 0x2078},
        {0x1080, 0x3000}, {0x1084, 0x3008},
        {0x3000, 0x4000}, {0x3004, 0x4100},
        {0x9004, 0xa000}, {0x9008, 0xa010},
        {0x1008, 0x5000}, {0x100c, 0x5010},
        {0x5000, 0xffffffff}, {0x5004, 1},
        {0x1060, 0x6000}, {0x1064, 0x6320}};
    std::vector<std::string> calls;
    std::uint32_t collection_size = 2;
    std::uint32_t found = 0x5000;
    std::uint32_t converted_key = 0xffffffff;
    bool valid = true;
    pal4::GameDataEquipRecord copied{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x107c, "collection address");
        calls.emplace_back("collection size");
        return collection_size;
    }
    void LogBind(std::string_view prefix, std::uint32_t table_name) override {
        Check(table_name == 0x1034, "table name address");
        calls.emplace_back(prefix);
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x106c && source == 0x1014,
              "bind assignment offsets");
        calls.emplace_back("assign");
    }
    void InitializeStructures(std::uint32_t object) override {
        Check(object == 0x106c, "binding initialization address");
        calls.emplace_back("initialize binding");
    }
    void BindEffects(std::uint32_t object) override {
        Check(object == 0x106c, "binding address");
        calls.emplace_back("bind");
    }
    void InitializeRecord(pal4::GameDataEquipRecord& record) override {
        record.fill(0);
        calls.emplace_back("initialize record");
    }
    void DestroyRecord(pal4::GameDataEquipRecord& record) override {
        Check(record[36] == converted_key || record[36] == 0,
              "temporary record remains accessible at cleanup");
        calls.emplace_back("destroy record");
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x9000 && index < 2, "property index");
        calls.push_back("property " + std::to_string(index));
        return 0xb000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataEquipRecord& record,
                      std::uint32_t property) override {
        Check(field == 0x4000 + 0x100 * (property - 0xb000),
              "field and property pair");
        calls.push_back("convert " + std::to_string(property - 0xb000));
        record[property - 0xb000] = property;
        record[36] = converted_key;
    }
    bool CheckMemberRange(
        const pal4::GameDataEquipRecord& record) override {
        Check(record[36] == converted_key, "member check receives key word 36");
        calls.emplace_back("check range");
        return valid;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t key) override {
        Check(begin == 0x5000 && end == 0x5010 && key == converted_key,
              "signed key search arguments");
        calls.emplace_back("find");
        return found;
    }
    void CopyRecord(std::uint32_t destination,
                    const pal4::GameDataEquipRecord& record) override {
        Check(destination == 0x6190, "indexed 400-byte record address");
        copied = record;
        calls.emplace_back("copy");
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataEquipRecord& record) override {
        Check(array == 0x105c && end == 0x6320 &&
                  record[36] == converted_key,
              "record insertion address and key");
        calls.emplace_back("insert record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t position,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && position == found &&
                  key == converted_key && index == 2,
              "key insertion uses two existing 400-byte records");
        calls.emplace_back("insert key");
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataEquipRecord& record) override {
        Check(array == 0x105c && end == 0x6320 &&
                  record[0] == 0xb000 && record[1] == 0xb001 &&
                  record[36] == converted_key,
              "append gets complete 400-byte record");
        calls.emplace_back("append");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x9000, "dump row context");
        calls.emplace_back("dump");
        return ": equip row 17";
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x1058 && value == 1, "dirty flag offset");
        calls.emplace_back("flag");
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x1000, "refresh table address");
        calls.emplace_back("refresh");
        return 0x12345678;
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected runtime call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckBoundRow() {
    Runtime valid;
    pal4::GameDataEquipRecord record{};
    Check(pal4::GameDataEquipTable::ParseBoundRow(
              0x106c, 0x9000, record, valid) &&
              record[0] == 0xb000 && record[1] == 0xb001 &&
              record[36] == 0xffffffff &&
              At(valid, "convert 0") < At(valid, "convert 1"),
          "bound row dispatches each field through its property");

    Runtime mismatch;
    mismatch.words[0x9008] = 0xa008;
    pal4::GameDataEquipRecord rejected{};
    Check(!pal4::GameDataEquipTable::ParseBoundRow(
              0x106c, 0x9000, rejected, mismatch) &&
              rejected == pal4::GameDataEquipRecord{} &&
              std::find(mismatch.calls.begin(), mismatch.calls.end(),
                        "property 0") == mismatch.calls.end(),
          "row count mismatch returns false without conversion");

    Runtime empty;
    empty.words[0x9004] = 0;
    empty.words[0x1080] = 0;
    Check(pal4::GameDataEquipTable::ParseBoundRow(
              0x106c, 0x9000, rejected, empty),
          "two null ranges both count as empty");
}

void CheckAddOrReplace() {
    Runtime replacement;
    Check(pal4::GameDataEquipTable::AddOrReplace(
              0x1000, 0x9000, replacement) == 1 &&
              replacement.copied[36] == 0xffffffff &&
              At(replacement, "initialize record") <
                  At(replacement, "convert 0") &&
              At(replacement, "copy") <
                  At(replacement, "destroy record") &&
              replacement.calls.back() == "destroy record",
          "existing signed ID replaces indexed 400-byte record");

    Runtime insertion;
    insertion.found = 0x5010;
    Check(pal4::GameDataEquipTable::AddOrReplace(
              0x1000, 0x9000, insertion) == 1 &&
              At(insertion, "insert record") < At(insertion, "insert key") &&
              insertion.calls.back() == "destroy record",
          "new ID inserts value before key and destroys temporary text");

    Runtime signed_order;
    signed_order.converted_key = 0x80000000;
    Check(pal4::GameDataEquipTable::AddOrReplace(
              0x1000, 0x9000, signed_order) == 1 &&
              At(signed_order, "insert record") <
                  At(signed_order, "insert key"),
          "negative signed ID before minus one inserts a new key");

    Runtime rebind;
    rebind.words[0x101c] = 0x203c;
    Check(pal4::GameDataEquipTable::AddOrReplace(
              0x1000, 0x9000, rebind) == 1 &&
              At(rebind, "beginBindField, table:") <
                  At(rebind, "assign") &&
              At(rebind, "assign") < At(rebind, "initialize binding") &&
              At(rebind, "initialize binding") < At(rebind, "bind") &&
              At(rebind, "bind") < At(rebind, "endBindField, table:") &&
              At(rebind, "endBindField, table:") <
                  At(rebind, "initialize record"),
          "changed field count rebinds before constructing a row");
}

void CheckAppendAndErrors() {
    Runtime appended;
    pal4::GameDataEquipTable::Append(0x1000, 0x9000, appended);
    Check(At(appended, "append") < At(appended, "destroy record") &&
              appended.calls.back() == "destroy record",
          "append releases its prepared text after native insertion");

    for (int failure = 0; failure < 3; ++failure) {
        Runtime invalid;
        if (failure == 0) invalid.words[0x9008] = 0xa008;
        if (failure == 1) invalid.words[0x1084] = 0x3004;
        if (failure == 2) invalid.valid = false;
        bool thrown = false;
        try {
            (void)pal4::GameDataEquipTable::AddOrReplace(
                0x1000, 0x9000, invalid);
        } catch (const std::runtime_error& error) {
            thrown = std::string_view(error.what()) ==
                     "ERROR <dbRecordBindTable< Record >::addRow >: equip row 17";
        }
        Check(thrown && At(invalid, "dump") <
                            At(invalid, "destroy record") &&
                  invalid.calls.back() == "destroy record",
              "parse or member error dumps row and releases temp record");
    }

    Runtime invalid_append;
    invalid_append.valid = false;
    bool thrown = false;
    try {
        pal4::GameDataEquipTable::Append(0x1000, 0x9000, invalid_append);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && invalid_append.calls.back() == "destroy record",
          "append member error releases temp record on throw");

    Runtime wrong_size;
    wrong_size.words[0x9008] = 0xa008;
    thrown = false;
    try {
        pal4::GameDataEquipTable::Append(0x1000, 0x9000, wrong_size);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && wrong_size.calls.back() == "destroy record",
          "append row-size error releases temp record before propagation");
}

void CheckRefresh() {
    Runtime runtime;
    Check(pal4::GameDataEquipTable::MarkAndRefresh(
              0x1000, runtime) == 0x12345678 &&
              runtime.calls == std::vector<std::string>{"flag", "refresh"},
          "dirty byte precedes refresh and returns its result");
}

}  // namespace

int main() {
    try {
        CheckBoundRow();
        CheckAddOrReplace();
        CheckAppendAndErrors();
        CheckRefresh();
        std::cout << "game_data_equip_table_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
