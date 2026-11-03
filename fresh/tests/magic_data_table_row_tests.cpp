#include "pal4/magic_data_table.h"

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

struct Call {
    std::string name;
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    bool operator==(const Call&) const = default;
};

struct RowRuntime final : pal4::IMagicDataRowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::int32_t bound_count = 2;
    std::uint32_t key = 5;
    std::uint32_t position = 0x3008;
    std::uint32_t type = 4;
    std::uint32_t element = 6;
    std::uint32_t category = 5;
    bool parse_result = true;
    bool fail_insert_key = false;
    bool fail_append = false;

    RowRuntime() {
        words[0x1018] = 0x2000;
        words[0x101c] = 0x2000 + 120;
        words[0x1008] = 0x3000;
        words[0x100c] = 0x3010;
        words[0x3008] = 8;
        words[0x300c] = 3;
        words[0x1060] = 0x4000;
        words[0x1064] = 0x4000 + 2 * 152;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x1060) calls.push_back({"record_begin"});
        if (address == 0x1064) calls.push_back({"record_end"});
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
    void InitializeRecord(pal4::MagicDataRecord& record) override {
        calls.push_back({"initialize"});
        record.fill(0);
    }
    std::uint8_t CleanupTemporaryRecord(
        pal4::MagicDataRecord&) override {
        calls.push_back({"cleanup"});
        return 0x5a;
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::MagicDataRecord& record) override {
        calls.push_back({"parse", binding, row});
        record[0] = key;
        record[1] = type;
        record[2] = element;
        record[20] = category;
        return parse_result;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t value) override {
        Check(value == key, "index key");
        calls.push_back({"find", begin, end});
        return position;
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::MagicDataRecord& record) override {
        calls.push_back({"insert_record", array, end});
        Check(record[0] == key, "inserted record ID");
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t value, std::uint32_t index) override {
        Check(value == key && at == position, "inserted key and position");
        calls.push_back({"insert_key", array, index});
        if (fail_insert_key) throw std::runtime_error("index failed");
    }
    void AssignRecord(std::uint32_t destination,
                      const pal4::MagicDataRecord& record) override {
        calls.push_back({"assign_record", destination, record[0]});
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::MagicDataRecord& record) override {
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

struct RefreshRuntime final : pal4::IMagicDataRefreshRuntime {
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

std::size_t FirstCall(const RowRuntime& runtime, const std::string& name) {
    for (std::size_t index = 0; index < runtime.calls.size(); ++index)
        if (runtime.calls[index].name == name) return index;
    throw std::runtime_error("missing expected call");
}

void CheckInsertReplaceAppend() {
    RowRuntime inserted;
    inserted.bound_count = 1;
    Check(pal4::MagicDataTable::AddOrReplace(
              0x1000, 0x5000, inserted) == 1 &&
              inserted.calls[0] == Call{"count", 0x107c} &&
              inserted.calls[1] == Call{"begin_bind", 0x1034} &&
              inserted.calls[2] ==
                  Call{"assign_effects", 0x106c, 0x1014} &&
              inserted.calls[3] == Call{"properties", 0x106c} &&
              inserted.calls[4] == Call{"bind", 0x106c} &&
              inserted.calls[5] == Call{"end_bind", 0x1034} &&
              FirstCall(inserted, "record_begin") <
                  FirstCall(inserted, "find") &&
              inserted.calls[inserted.calls.size() - 3] ==
                  Call{"insert_record", 0x105c, 0x4000 + 2 * 152} &&
              inserted.calls[inserted.calls.size() - 2] ==
                  Call{"insert_key", 0x1004, 2} &&
              inserted.calls.back() == Call{"cleanup"},
          "binding mismatch inserts 152-byte row and its physical index");

    RowRuntime replaced;
    replaced.words[0x3008] = 5;
    Check(pal4::MagicDataTable::AddOrReplace(
              0x1000, 0x5000, replaced) == 1 &&
              !HasCall(replaced, "begin_bind") &&
              FirstCall(replaced, "record_begin") <
                  FirstCall(replaced, "find") &&
              replaced.calls[replaced.calls.size() - 2] ==
                  Call{"assign_record", 0x4000 + 3 * 152, 5} &&
              replaced.calls.back() == Call{"cleanup"},
          "matching key replaces physical record after counting rows");

    RowRuntime appended;
    Check(pal4::MagicDataTable::Append(
              0x1000, 0x5000, appended) == 0x5a &&
              appended.calls == std::vector<Call>{
                  {"initialize"}, {"parse", 0x106c, 0x5000},
                  {"record_end"}, {"append", 0x105c, 0x4000 + 2 * 152},
                  {"cleanup"}},
          "append parses, validates and forwards final cleanup result");

    RefreshRuntime refresh;
    Check(pal4::MagicDataTable::MarkAndRefresh(
              0x1000, refresh) == 0x1234 &&
              refresh.calls == std::vector<Call>{
                  {"dirty", 0x1058, 1}, {"refresh", 0x1000}},
          "dirty byte precedes refresh and return is forwarded");
}

void CheckCountAndInvalidRows() {
    RowRuntime runtime;
    Check(pal4::MagicDataTable::RecordCount(
              0x105c, runtime) == 2,
          "record count uses 152-byte stride");
    runtime.words[0x1060] = 0;
    Check(pal4::MagicDataTable::RecordCount(
              0x105c, runtime) == 0,
          "null begin yields zero regardless of stored end");
    runtime.words[0x1060] = 0x4000;
    runtime.words[0x1064] = 0x4000 - 152;
    Check(pal4::MagicDataTable::RecordCount(
              0x105c, runtime) == -1,
          "signed negative span follows native division");

    for (int case_index = 0; case_index < 7; ++case_index) {
        RowRuntime invalid;
        switch (case_index) {
        case 0: invalid.parse_result = false; break;
        case 1: invalid.element = 7; break;
        case 2: invalid.element = 0xffffffff; break;
        case 3: invalid.type = 5; break;
        case 4: invalid.type = 0xffffffff; break;
        case 5: invalid.category = 6; break;
        case 6: invalid.category = 0xffffffff; break;
        }
        for (bool append : {false, true}) {
            invalid.calls.clear();
            try {
                if (append)
                    static_cast<void>(pal4::MagicDataTable::Append(
                        0x1000, 0x5000, invalid));
                else
                    static_cast<void>(pal4::MagicDataTable::AddOrReplace(
                        0x1000, 0x5000, invalid));
                Check(false, "invalid Magic row must throw");
            } catch (const std::runtime_error& error) {
                Check(std::string(error.what()) ==
                          "ERROR <dbRecordBindTable< Record >::addRow > row" &&
                          HasCall(invalid, "dump") &&
                          !HasCall(invalid, "find") &&
                          invalid.calls.back() == Call{"cleanup"},
                      "invalid signed bounds dump and clean temporary row");
            }
        }
    }
}

void CheckRuntimeFailures() {
    RowRuntime insert_failure;
    insert_failure.fail_insert_key = true;
    try {
        static_cast<void>(pal4::MagicDataTable::AddOrReplace(
            0x1000, 0x5000, insert_failure));
        Check(false, "index insertion failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "index failed" &&
                  HasCall(insert_failure, "insert_record") &&
                  insert_failure.calls.back() == Call{"cleanup"},
              "temporary row is cleaned after index failure");
    }
    RowRuntime append_failure;
    append_failure.fail_append = true;
    try {
        static_cast<void>(pal4::MagicDataTable::Append(
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
        CheckCountAndInvalidRows();
        CheckRuntimeFailures();
        std::cout << "magic_data_table_row_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
