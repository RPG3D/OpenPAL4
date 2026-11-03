#include "pal4/utilities_world_table.h"

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

struct RowRuntime final : pal4::IUtilitiesWorldRowRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1018, 0x2000}, {0x101c, 0x2078},
        {0x1008, 0x3000}, {0x100c, 0x3010},
        {0x3000, 3}, {0x3004, 0}, {0x3008, 9}, {0x300c, 2},
        {0x1060, 0x4000}, {0x1064, 0x4078}};
    std::vector<std::string> calls;
    std::int32_t bound_count = 1;
    std::uint32_t key = 7;
    bool parse_ok = true;
    bool fail_insert = false;
    std::uint32_t record_address = 0;
    std::uint32_t index_address = 0;
    std::uint32_t physical_index = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::int32_t CollectionSize(std::uint32_t collection) override {
        Check(collection == 0x107c, "field collection offset");
        return bound_count;
    }
    void LogBind(std::string_view prefix, std::uint32_t name) override {
        Check(name == 0x1034, "table name offset");
        calls.emplace_back(prefix == "beginBindField, table:"
                               ? "begin" : "end");
    }
    void AssignEffects(std::uint32_t to, std::uint32_t from) override {
        Check(to == 0x106c && from == 0x1014, "effect binding offsets");
        calls.emplace_back("assign");
    }
    void RegisterWorldFields(std::uint32_t binding) override {
        Check(binding == 0x106c, "World field binding offset");
        calls.emplace_back("world_fields");
    }
    void BindEffects(std::uint32_t binding) override {
        Check(binding == 0x106c, "bind offset");
        calls.emplace_back("bind");
    }
    void InitializeRecord(pal4::UtilitiesWorldRecord& record) override {
        record.fill(0);
        calls.emplace_back("init");
    }
    void DestroyTemporaryRecord(pal4::UtilitiesWorldRecord&) override {
        calls.emplace_back("cleanup");
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::UtilitiesWorldRecord& record) override {
        Check(binding == 0x106c && row == 0x5000, "parse arguments");
        calls.emplace_back("parse");
        record[0] = key;
        record[1] = 0x1111;
        record[6] = 0x6666;
        return parse_ok;
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::UtilitiesWorldRecord& record) override {
        Check(array == 0x105c && record[1] == 0x1111 &&
                  record[6] == 0x6666,
              "insert receives full 40-byte row");
        record_address = end;
        calls.emplace_back("insert_row");
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t value, std::uint32_t index) override {
        Check(array == 0x1004 && value == key, "key insertion arguments");
        index_address = at;
        physical_index = index;
        calls.emplace_back("insert_key");
        if (fail_insert) throw std::runtime_error("insert failed");
    }
    void AssignRecord(std::uint32_t to,
                      const pal4::UtilitiesWorldRecord& record) override {
        Check(record[1] == 0x1111 && record[6] == 0x6666,
              "replacement receives full row");
        record_address = to;
        calls.emplace_back("replace");
    }
    std::string DumpRow(std::uint32_t table, std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x5000, "error dump arguments");
        calls.emplace_back("dump");
        return " row=5";
    }
};

void CheckRows() {
    RowRuntime inserted;
    Check(pal4::UtilitiesWorldTable::AddOrReplace(
              0x1000, 0x5000, inserted) == 1,
          "insert result");
    Check(inserted.calls == std::vector<std::string>{
              "begin", "assign", "world_fields", "bind", "end",
              "init", "parse", "insert_row", "insert_key", "cleanup"},
          "rebind and insert order");
    Check(inserted.record_address == 0x4078 &&
              inserted.index_address == 0x3008 &&
              inserted.physical_index == 3,
          "40-byte physical row and signed index location");

    RowRuntime replaced;
    replaced.bound_count = 2;
    replaced.key = 9;
    Check(pal4::UtilitiesWorldTable::AddOrReplace(
              0x1000, 0x5000, replaced) == 1 &&
              replaced.record_address == 0x4050 &&
              replaced.calls == std::vector<std::string>{
                  "init", "parse", "replace", "cleanup"},
          "matching key replaces its 40-byte physical row");

    RowRuntime negative;
    negative.bound_count = 2;
    negative.key = static_cast<std::uint32_t>(-2);
    static_cast<void>(pal4::UtilitiesWorldTable::AddOrReplace(
        0x1000, 0x5000, negative));
    Check(negative.index_address == 0x3000,
          "signed negative key precedes positive keys");

    RowRuntime failed;
    failed.parse_ok = false;
    try {
        static_cast<void>(pal4::UtilitiesWorldTable::AddOrReplace(
            0x1000, 0x5000, failed));
        Check(false, "parse error must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow > row=5" &&
                  failed.calls.back() == "cleanup",
              "parse error dumps and cleans row");
    }

    RowRuntime interrupted;
    interrupted.bound_count = 2;
    interrupted.fail_insert = true;
    try {
        static_cast<void>(pal4::UtilitiesWorldTable::AddOrReplace(
            0x1000, 0x5000, interrupted));
        Check(false, "insert error must throw");
    } catch (const std::runtime_error&) {
        Check(interrupted.calls.back() == "cleanup",
              "insert failure cleans temporary row");
    }
}

struct AppendRuntime final : pal4::IUtilitiesWorldAppendRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1080, 0x6000}, {0x6000, 0x6100}, {0x6004, 0x6200},
        {0x1064, 0x4078}};
    std::vector<std::string> calls;
    std::uint32_t width = 2;
    bool fail_convert = false;
    std::uint32_t appended_end = 0;
    std::uint32_t converted = 0;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x7000, "append row argument");
        return width;
    }
    std::uint32_t CollectionSize(std::uint32_t collection) override {
        Check(collection == 0x107c, "append collection offset");
        return 2;
    }
    void InitializeRecord(pal4::UtilitiesWorldRecord& record) override {
        record.fill(0);
        calls.emplace_back("init");
    }
    std::uint32_t CleanupTemporaryRecord(
        pal4::UtilitiesWorldRecord&) override {
        calls.emplace_back("cleanup");
        return 0x77;
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x7000 && index < 2, "property position");
        return 0x7100 + index;
    }
    void ConvertField(std::uint32_t binding,
                      pal4::UtilitiesWorldRecord& record,
                      std::uint32_t property) override {
        Check(binding == 0x6100 + converted * 0x100 &&
                  property == 0x7100 + converted,
              "field and property pairing");
        calls.emplace_back("convert");
        record[converted++] = property;
        if (fail_convert) throw std::runtime_error("conversion failed");
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::UtilitiesWorldRecord& record) override {
        Check(array == 0x105c && record[0] == 0x7100 &&
                  record[1] == 0x7101,
              "converted World row reaches storage");
        appended_end = end;
        calls.emplace_back("append");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x7000, "append error dump");
        calls.emplace_back("dump");
        return " row=7";
    }
};

void CheckAppend() {
    AppendRuntime normal;
    Check(pal4::UtilitiesWorldTable::Append(0x1000, 0x7000, normal) ==
              0x77 && normal.appended_end == 0x4078 &&
              normal.calls == std::vector<std::string>{
                  "init", "convert", "convert", "append", "cleanup"},
          "append converts both fields and returns cleanup result");

    AppendRuntime mismatch;
    mismatch.width = 1;
    try {
        static_cast<void>(pal4::UtilitiesWorldTable::Append(
            0x1000, 0x7000, mismatch));
        Check(false, "width mismatch must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow > row=7" &&
                  mismatch.calls == std::vector<std::string>{
                      "init", "dump", "cleanup"},
              "width mismatch reports row and cleans temporary data");
    }

    AppendRuntime interrupted;
    interrupted.fail_convert = true;
    try {
        static_cast<void>(pal4::UtilitiesWorldTable::Append(
            0x1000, 0x7000, interrupted));
        Check(false, "conversion failure must throw");
    } catch (const std::runtime_error&) {
        Check(interrupted.calls.back() == "cleanup" &&
                  interrupted.appended_end == 0,
              "conversion failure cleans without appending");
    }
}

struct RefreshRuntime final : pal4::IUtilitiesWorldRefreshRuntime {
    std::vector<std::string> calls;
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x1058 && value == 1, "dirty byte");
        calls.emplace_back("mark");
    }
    std::uint32_t Refresh(std::uint32_t object) override {
        Check(object == 0x1000, "refresh object");
        calls.emplace_back("refresh");
        return 0x1234;
    }
};

}  // namespace

int main() {
    try {
        CheckRows();
        CheckAppend();
        RefreshRuntime refresh;
        Check(pal4::UtilitiesWorldTable::MarkAndRefresh(
                  0x1000, refresh) == 0x1234 &&
                  refresh.calls == std::vector<std::string>{
                      "mark", "refresh"},
              "dirty flag precedes refresh");
        std::cout << "utilities_world_row_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
