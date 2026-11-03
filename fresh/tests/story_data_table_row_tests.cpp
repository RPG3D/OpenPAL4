#include "pal4/story_data_table.h"
#include "pal4/story_desc_data_table.h"

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

struct Runtime final : pal4::IStoryDataRowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::int32_t bound_count = 2;
    std::int32_t row_size = 2;
    std::uint32_t key = 7;
    std::uint32_t position = 0x8100;
    std::uint32_t cleanup_result = 0x45;
    bool parse_ok = true;
    bool fail_convert = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::int32_t CollectionSize(std::uint32_t collection) override {
        Check(collection == 0x707c, "bound collection address");
        calls.push_back("size");
        return bound_count;
    }
    std::int32_t RowSize(std::uint32_t row) override {
        calls.push_back("row_size:" + std::to_string(row));
        return row_size;
    }
    void LogBind(std::string_view marker,
                 std::uint32_t name) override {
        calls.push_back("log:" + std::string(marker) + ":" +
                        std::to_string(name));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        calls.push_back("assign_effects:" + std::to_string(destination) +
                        ":" + std::to_string(source));
    }
    void ProcessPropertyRegistry(std::uint32_t binding) override {
        calls.push_back("registry:" + std::to_string(binding));
    }
    void BindEffects(std::uint32_t binding) override {
        calls.push_back("bind:" + std::to_string(binding));
    }
    void InitializeRecord(pal4::StoryDataRecord& record) override {
        record.fill(0);
        calls.push_back("init_record");
    }
    std::uint32_t CleanupTemporaryRecord(
        pal4::StoryDataRecord&) override {
        calls.push_back("cleanup_record");
        return cleanup_result;
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::StoryDataRecord& record) override {
        calls.push_back("parse:" + std::to_string(binding) + ":" +
                        std::to_string(row));
        record[0] = key;
        return parse_ok;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t search_key) override {
        calls.push_back("find:" + std::to_string(begin) + ":" +
                        std::to_string(end) + ":" +
                        std::to_string(search_key));
        return position;
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::StoryDataRecord& record) override {
        calls.push_back("insert_record:" + std::to_string(array) + ":" +
                        std::to_string(end) + ":" +
                        std::to_string(record[0]));
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::StoryDataRecord& record) override {
        calls.push_back("append_record:" + std::to_string(array) + ":" +
                        std::to_string(end) + ":" +
                        std::to_string(record[0]));
    }
    void InsertKey(std::uint32_t array, std::uint32_t insert_at,
                   std::uint32_t value) override {
        calls.push_back("insert_key:" + std::to_string(array) + ":" +
                        std::to_string(insert_at) + ":" +
                        std::to_string(value));
    }
    void AssignRecord(std::uint32_t destination,
                      const pal4::StoryDataRecord& record) override {
        calls.push_back("assign_record:" +
                        std::to_string(destination) + ":" +
                        std::to_string(record[0]));
    }
    std::uint32_t FieldAt(std::uint32_t fields,
                          std::uint32_t index) override {
        calls.push_back("field:" + std::to_string(fields) + ":" +
                        std::to_string(index));
        return 0xa000 + index;
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        calls.push_back("property:" + std::to_string(row) + ":" +
                        std::to_string(index));
        return 0xb000 + index;
    }
    void ConvertField(std::uint32_t field, pal4::StoryDataRecord& record,
                      std::uint32_t property) override {
        calls.push_back("convert:" + std::to_string(field) + ":" +
                        std::to_string(property));
        record[0] = key;
        if (fail_convert) throw std::runtime_error("convert failed");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        calls.push_back("dump:" + std::to_string(table) + ":" +
                        std::to_string(row));
        return "row-info";
    }
};

void Seed(Runtime& runtime) {
    constexpr std::uint32_t table = 0x7000;
    runtime.words[table + 24] = 0x8000;
    runtime.words[table + 28] = 0x8078;
    runtime.words[table + 8] = 0x8100;
    runtime.words[table + 12] = 0x8100;
    runtime.words[table + 96] = 0x9000;
    runtime.words[table + 100] = 0x9048;
}

void CheckAddOrReplace() {
    constexpr std::uint32_t table = 0x7000;
    constexpr std::uint32_t row = 0x7800;
    Runtime insert;
    Seed(insert);
    insert.bound_count = 1;
    Check(pal4::StoryDataTable::AddOrReplace(table, row, insert) == 1 &&
              insert.calls == std::vector<std::string>{
                  "size", "log:beginBindField, table::28724",
                  "assign_effects:28780:28692", "registry:28780",
                  "bind:28780", "log:endBindField, table::28724",
                  "init_record", "parse:28780:30720",
                  "find:33024:33024:7", "insert_record:28764:36936:7",
                  "insert_key:28676:33024:7", "cleanup_record"},
          "binding refresh precedes parsed insertion and index update");

    Runtime replace;
    Seed(replace);
    replace.words[table + 12] = 0x8108;
    replace.words[0x8100] = 7;
    replace.words[0x8104] = 2;
    Check(pal4::StoryDataTable::AddOrReplace(table, row, replace) == 1 &&
              replace.calls == std::vector<std::string>{
                  "size", "init_record", "parse:28780:30720",
                  "find:33024:33032:7", "assign_record:36936:7",
                  "cleanup_record"},
          "existing signed ID assigns the indexed physical record");

    Runtime negative;
    Seed(negative);
    negative.key = 0xffffffff;
    negative.words[table + 12] = 0x8108;
    negative.words[0x8100] = 1;
    Check(pal4::StoryDataTable::AddOrReplace(table, row, negative) == 1 &&
              negative.calls[negative.calls.size() - 3] ==
                  "insert_record:28764:36936:4294967295" &&
              negative.calls[negative.calls.size() - 2] ==
                  "insert_key:28676:33024:4294967295",
          "signed negative key inserts before positive key");

    Runtime failed;
    Seed(failed);
    failed.parse_ok = false;
    try {
        static_cast<void>(pal4::StoryDataTable::AddOrReplace(
            table, row, failed));
        Check(false, "parse failure must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow >row-info" &&
                  failed.calls[3] == "dump:28672:30720" &&
                  failed.calls.back() == "cleanup_record",
              "parse failure dumps row and releases temporary record");
    }
}

void CheckAppend() {
    constexpr std::uint32_t table = 0x7000;
    constexpr std::uint32_t row = 0x7800;
    Runtime runtime;
    Seed(runtime);
    const auto append_result = pal4::StoryDataTable::Append(table, row, runtime);
    const std::vector<std::string> expected = {
                  "init_record", "row_size:30720", "size",
                  "size", "field:28800:0", "property:30720:0",
                  "convert:40960:45056", "size", "field:28800:1",
                  "property:30720:1", "convert:40961:45057", "size",
                  "append_record:28764:36936:7", "cleanup_record"};
    Check(append_result == 0x45 && runtime.calls == expected,
          "matching fields convert in order before append and final cleanup");

    Runtime empty;
    Seed(empty);
    empty.bound_count = 0;
    empty.row_size = 0;
    Check(pal4::StoryDataTable::Append(table, row, empty) == 0x45 &&
              empty.calls.back() == "cleanup_record" &&
              empty.calls[3] == "size" &&
              empty.calls[4] == "append_record:28764:36936:0",
          "zero-field row still appends a default record");

    Runtime mismatch;
    Seed(mismatch);
    mismatch.row_size = 1;
    try {
        static_cast<void>(pal4::StoryDataTable::Append(
            table, row, mismatch));
        Check(false, "size mismatch must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow >row-info" &&
                  mismatch.calls == std::vector<std::string>{
                      "init_record", "row_size:30720", "size",
                      "dump:28672:30720", "cleanup_record"},
              "size mismatch dumps row before temporary cleanup");
    }

    Runtime conversion_failure;
    Seed(conversion_failure);
    conversion_failure.fail_convert = true;
    try {
        static_cast<void>(pal4::StoryDataTable::Append(
            table, row, conversion_failure));
        Check(false, "conversion failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "convert failed" &&
                  conversion_failure.calls.back() == "cleanup_record",
              "conversion failure stops before append and cleans record");
    }
}

struct RefreshRuntime final : pal4::IStoryDataRefreshRuntime {
    std::vector<std::string> calls;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        calls.push_back("refresh:" + std::to_string(table));
        return 0x5151;
    }
};

void CheckRefresh() {
    RefreshRuntime runtime;
    Check(pal4::StoryDataTable::MarkAndRefresh(0x7000, runtime) == 0x5151 &&
              runtime.calls == std::vector<std::string>{
                  "write:28760:1", "refresh:28672"},
          "dirty byte is set before refresh and result is forwarded");
}

void CheckStoryDescRows() {
    constexpr std::uint32_t table = 0x7000;
    constexpr std::uint32_t row = 0x7800;
    Runtime insert;
    Seed(insert);
    Check(pal4::StoryDescDataTable::AddOrReplace(table, row, insert) == 1 &&
              insert.calls[3] == "find:33024:33024:7" &&
              insert.calls[4] == "insert_record:28764:36936:7" &&
              insert.calls[5] == "insert_key:28676:33024:7",
          "StoryDesc add path uses the same 36-byte key and row layout");

    Runtime append;
    Seed(append);
    Check(pal4::StoryDescDataTable::Append(table, row, append) == 0x45 &&
              append.calls[append.calls.size() - 2] ==
                  "append_record:28764:36936:7" &&
              append.calls.back() == "cleanup_record",
          "StoryDesc append converts fields before final cleanup");

    Runtime invalid;
    Seed(invalid);
    invalid.parse_ok = false;
    try {
        static_cast<void>(pal4::StoryDescDataTable::AddOrReplace(
            table, row, invalid));
        Check(false, "StoryDesc parse failure must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow >row-info" &&
                  invalid.calls.back() == "cleanup_record",
              "StoryDesc invalid row releases temporary record");
    }

    RefreshRuntime refresh;
    Check(pal4::StoryDescDataTable::MarkAndRefresh(table, refresh) ==
              0x5151 &&
              refresh.calls == std::vector<std::string>{
                  "write:28760:1", "refresh:28672"},
          "StoryDesc dirty flag precedes refresh");
}

}  // namespace

int main() {
    try {
        CheckAddOrReplace();
        CheckAppend();
        CheckRefresh();
        CheckStoryDescRows();
        std::cout << "story_data_table_row_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
