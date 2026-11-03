#include "pal4/skill_data_table.h"

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

struct AppendRuntime final : pal4::ISkillDataAppendRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t count = 2;
    std::uint32_t row_size = 2;
    std::uint32_t type = 3;
    std::uint32_t trigger = 14;
    bool fail_convert = false;
    bool fail_append = false;
    std::uint32_t appended_array = 0;
    std::uint32_t appended_end = 0;
    pal4::SkillDataRecord appended{};

    AppendRuntime() {
        words[0x1080] = 0x2000;
        words[0x2000] = 0x3000;
        words[0x2004] = 0x3004;
        words[0x1064] = 0x4000;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x107c, "binding count address");
        calls.push_back("count");
        return count;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x5000, "row size address");
        calls.push_back("row_size");
        return row_size;
    }
    void InitializeRecord(pal4::SkillDataRecord& record) override {
        calls.push_back("initialize");
        record.fill(0);
        record[1] = type;
        record[2] = trigger;
    }
    std::uint8_t CleanupTemporaryRecord(
        pal4::SkillDataRecord&) override {
        calls.push_back("cleanup");
        return 0x5a;
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x5000, "property row address");
        calls.push_back("property:" + std::to_string(index));
        return 0x6000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::SkillDataRecord& record,
                      std::uint32_t property) override {
        calls.push_back("convert:" + std::to_string(field));
        Check(field == 0x3000 + 4 * (property - 0x6000),
              "field and property index must agree");
        record[0] = 7;
        if (fail_convert) throw std::runtime_error("convert failed");
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::SkillDataRecord& record) override {
        calls.push_back("append");
        appended_array = array;
        appended_end = end;
        appended = record;
        if (fail_append) throw std::runtime_error("append failed");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x5000, "dump arguments");
        calls.push_back("dump");
        return " row";
    }
};

struct RefreshRuntime final : pal4::ISkillDataRefreshRuntime {
    std::vector<std::string> calls;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        Check(address == 0x1058 && value == 1, "dirty flag address");
        calls.push_back("dirty");
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x1000, "refresh table address");
        calls.push_back("refresh");
        return 0x1234;
    }
};

void CheckAppend() {
    AppendRuntime runtime;
    Check(pal4::SkillDataTable::Append(0x1000, 0x5000, runtime) == 0x5a &&
              runtime.appended_array == 0x105c &&
              runtime.appended_end == 0x4000 &&
              runtime.appended[0] == 7 &&
              runtime.calls == std::vector<std::string>{
                  "initialize", "row_size", "count", "count",
                  "read:4224", "read:8192", "property:0", "convert:12288",
                  "count", "read:4224", "read:8196", "property:1",
                  "convert:12292", "count", "read:4196", "append", "cleanup"},
          "append converts each bound field, appends and cleans up");

    AppendRuntime empty;
    empty.count = 0;
    empty.row_size = 0;
    Check(pal4::SkillDataTable::Append(0x1000, 0x5000, empty) == 0x5a &&
              empty.appended_array == 0x105c &&
              empty.calls == std::vector<std::string>{
                  "initialize", "row_size", "count", "count",
                  "read:4196", "append", "cleanup"},
          "zero-field row skips conversion but still appends");
}

void CheckInvalidRowsAndExceptions() {
    for (int case_index = 0; case_index < 3; ++case_index) {
        AppendRuntime runtime;
        if (case_index == 0) runtime.row_size = 1;
        if (case_index == 1) runtime.type = 0xffffffff;
        if (case_index == 2) runtime.trigger = 15;
        try {
            static_cast<void>(pal4::SkillDataTable::Append(
                0x1000, 0x5000, runtime));
            Check(false, "invalid Skill row must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      "ERROR <dbRecordBindTable< Record >::addRow > row" &&
                      runtime.calls[runtime.calls.size() - 2] == "dump" &&
                      runtime.calls.back() == "cleanup" &&
                      runtime.appended_array == 0,
                  "invalid row dumps and destroys temporary record");
        }
    }
    for (int case_index = 0; case_index < 2; ++case_index) {
        AppendRuntime runtime;
        runtime.fail_convert = case_index == 0;
        runtime.fail_append = case_index == 1;
        try {
            static_cast<void>(pal4::SkillDataTable::Append(
                0x1000, 0x5000, runtime));
            Check(false, "runtime failure must propagate");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      (case_index == 0 ? "convert failed" : "append failed") &&
                      runtime.calls.back() == "cleanup",
                  "temporary record is cleaned during exceptional exit");
        }
    }
}

void CheckRefresh() {
    RefreshRuntime runtime;
    Check(pal4::SkillDataTable::MarkAndRefresh(0x1000, runtime) ==
              0x1234 &&
              runtime.calls == std::vector<std::string>{"dirty", "refresh"},
          "dirty flag precedes refresh and result is forwarded");
}

}  // namespace

int main() {
    try {
        CheckAppend();
        CheckInvalidRowsAndExceptions();
        CheckRefresh();
        std::cout << "skill_data_table_append_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
