#include "pal4/stunt_data_table.h"

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

struct Call {
    std::string name;
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    bool operator==(const Call&) const = default;
};

struct RowRuntime final : pal4::IStuntDataRowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::int32_t bound_count = 2;
    std::uint32_t key = 5;
    std::uint32_t position = 0x3008;
    std::uint32_t type = 6;
    std::uint32_t value80 = 0;
    std::uint32_t value84 = 2;
    std::uint32_t value88 = 5;
    bool parse_result = true;
    bool fail_insert_key = false;
    bool fail_append = false;

    RowRuntime() {
        words[0x1000 + 24] = 0x2000;
        words[0x1000 + 28] = 0x2000 + 120;
        words[0x1000 + 8] = 0x3000;
        words[0x1000 + 12] = 0x3010;
        words[0x3008] = 8;
        words[0x300c] = 3;
        words[0x1000 + 96] = 0x4000;
        words[0x1000 + 100] = 0x4000 + 2 * 208;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::int32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x107c, "bound collection address");
        calls.push_back({"count", address});
        return bound_count;
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t name) override {
        calls.push_back({prefix == "beginBindField, table:"
                             ? "begin_bind" : "end_bind", name});
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        calls.push_back({"assign_effects", destination, source});
    }
    void ProcessPropertyRegistry(std::uint32_t binding) override {
        calls.push_back({"properties", binding});
    }
    void BindEffects(std::uint32_t binding) override {
        calls.push_back({"bind", binding});
    }
    void InitializeRecord(pal4::StuntDataRecord& record) override {
        calls.push_back({"initialize"});
        record.fill(0);
    }
    std::uint8_t CleanupTemporaryRecord(
        pal4::StuntDataRecord&) override {
        calls.push_back({"cleanup"});
        return 0x5a;
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::StuntDataRecord& record) override {
        calls.push_back({"parse", binding, row});
        record[0] = key;
        record[1] = type;
        record[32] = value80;
        record[33] = value84;
        record[34] = value88;
        return parse_result;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t value) override {
        Check(value == key, "index key");
        calls.push_back({"find", begin, end});
        return position;
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::StuntDataRecord& record) override {
        calls.push_back({"insert_record", array, end});
        Check(record[0] == key, "inserted record ID");
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t value, std::uint32_t index) override {
        Check(value == key, "inserted key");
        calls.push_back({"insert_key", array, index});
        Check(at == position, "index position");
        if (fail_insert_key) throw std::runtime_error("index failed");
    }
    void AssignRecord(std::uint32_t destination,
                      const pal4::StuntDataRecord& record) override {
        calls.push_back({"assign_record", destination, record[0]});
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::StuntDataRecord& record) override {
        calls.push_back({"append", array, end});
        Check(record[0] == key, "appended record ID");
        if (fail_append) throw std::runtime_error("append failed");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        calls.push_back({"dump", table, row});
        return " row";
    }
};

struct RefreshRuntime final : pal4::IStuntDataRefreshRuntime {
    std::vector<Call> calls;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back({"dirty", address, value});
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        calls.push_back({"refresh", table});
        return 0x1234;
    }
};

bool HasCall(const RowRuntime& runtime, const std::string& name) {
    for (const auto& call : runtime.calls)
        if (call.name == name) return true;
    return false;
}

void CheckInsertReplaceAppend() {
    RowRuntime inserted;
    inserted.bound_count = 1;
    Check(pal4::StuntDataTable::AddOrReplace(
              0x1000, 0x5000, inserted) == 1 &&
              inserted.calls == std::vector<Call>{
                  {"count", 0x107c},
                  {"begin_bind", 0x1034},
                  {"assign_effects", 0x106c, 0x1014},
                  {"properties", 0x106c},
                  {"bind", 0x106c},
                  {"end_bind", 0x1034},
                  {"initialize"}, {"parse", 0x106c, 0x5000},
                  {"find", 0x3000, 0x3010},
                  {"insert_record", 0x105c, 0x4000 + 2 * 208},
                  {"insert_key", 0x1004, 2}, {"cleanup"}},
          "binding mismatch inserts 208-byte Stunt row with physical index");

    RowRuntime replaced;
    replaced.words[0x3008] = 5;
    Check(pal4::StuntDataTable::AddOrReplace(
              0x1000, 0x5000, replaced) == 1 &&
              !HasCall(replaced, "begin_bind") &&
              replaced.calls[4] ==
                  Call{"assign_record", 0x4000 + 3 * 208, 5} &&
              replaced.calls.back() == Call{"cleanup"},
          "matching key replaces indexed physical record");

    RowRuntime appended;
    Check(pal4::StuntDataTable::Append(
              0x1000, 0x5000, appended) == 0x5a &&
              appended.calls == std::vector<Call>{
                  {"initialize"}, {"parse", 0x106c, 0x5000},
                  {"append", 0x105c, 0x4000 + 2 * 208},
                  {"cleanup"}},
          "append parses, validates and forwards final cleanup result");
    RefreshRuntime refresh;
    Check(pal4::StuntDataTable::MarkAndRefresh(
              0x1000, refresh) == 0x1234 &&
              refresh.calls == std::vector<Call>{
                  {"dirty", 0x1058, 1}, {"refresh", 0x1000}},
          "refresh marks byte +88 before delegating");
}

void CheckInvalidAndFailure() {
    for (int case_index = 0; case_index < 9; ++case_index) {
        RowRuntime runtime;
        switch (case_index) {
        case 0: runtime.parse_result = false; break;
        case 1: runtime.value88 = 6; break;
        case 2: runtime.value88 = 0xffffffff; break;
        case 3: runtime.type = 7; break;
        case 4: runtime.type = 0xffffffff; break;
        case 5: runtime.value84 = 3; break;
        case 6: runtime.value84 = 0xffffffff; break;
        case 7: runtime.value80 = 0xffffffff; break;
        case 8: runtime.value80 = 0x80000000; break;
        }
        for (bool append : {false, true}) {
            runtime.calls.clear();
            try {
                if (append)
                    static_cast<void>(pal4::StuntDataTable::Append(
                        0x1000, 0x5000, runtime));
                else
                    static_cast<void>(pal4::StuntDataTable::AddOrReplace(
                        0x1000, 0x5000, runtime));
                Check(false, "invalid row must throw");
            } catch (const std::runtime_error& error) {
                Check(std::string(error.what()) ==
                          "ERROR <dbRecordBindTable< Record >::addRow > row" &&
                          HasCall(runtime, "dump") &&
                          !HasCall(runtime, "find") &&
                          runtime.calls.back() == Call{"cleanup"},
                      "invalid signed bounds dump and clean temporary row");
            }
        }
    }

    RowRuntime insert_failure;
    insert_failure.fail_insert_key = true;
    try {
        static_cast<void>(pal4::StuntDataTable::AddOrReplace(
            0x1000, 0x5000, insert_failure));
        Check(false, "index insertion failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "index failed" &&
                  HasCall(insert_failure, "insert_record") &&
                  insert_failure.calls.back() == Call{"cleanup"},
              "temporary row is cleaned after index insertion failure");
    }
    RowRuntime append_failure;
    append_failure.fail_append = true;
    try {
        static_cast<void>(pal4::StuntDataTable::Append(
            0x1000, 0x5000, append_failure));
        Check(false, "append failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "append failed" &&
                  append_failure.calls.back() == Call{"cleanup"},
              "temporary row is cleaned after append failure");
    }
}

}  // namespace

int main() {
    try {
        CheckInsertReplaceAppend();
        CheckInvalidAndFailure();
        std::cout << "stunt_data_table_row_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
