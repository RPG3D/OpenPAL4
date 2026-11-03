#include "pal4/game_data_object_table.h"

#include <array>
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

struct Runtime final : pal4::IGameDataObjectRowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::array<std::uint8_t, 9> text{
        '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    std::uint32_t collection_size = 0;
    std::uint32_t position = 0;
    std::uint32_t found_key = 0;
    std::uint32_t inserted_key = 0;
    std::uint32_t inserted_index = 0xffffffffu;
    std::uint32_t assigned_destination = 0;
    std::uint32_t saved_record_hash = 0;
    std::uint32_t status = 2;
    bool parse_result = true;
    bool null_text = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        events.push_back("size:" + std::to_string(address));
        return collection_size;
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t name) override {
        events.push_back("log:" + std::string(prefix) +
                         std::to_string(name));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        events.push_back("assign_effects:" + std::to_string(destination) +
                         ":" + std::to_string(source));
    }
    void RegisterField(std::uint32_t,
                       std::string_view name,
                       std::uint32_t,
                       pal4::GameDataFieldKind) override {
        events.push_back("field:" + std::string(name));
    }
    void BindEffects(std::uint32_t binding) override {
        events.push_back("bind:" + std::to_string(binding));
    }
    void InitializeRecord(pal4::GameDataObjectRecord&) override {
        events.push_back("initialize");
    }
    void DestroyTemporaryRecord(
        pal4::GameDataObjectRecord&) override {
        events.push_back("destroy_temporary");
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::GameDataObjectRecord& record) override {
        events.push_back("parse:" + std::to_string(binding) +
                         ":" + std::to_string(row));
        record[3] = null_text ? 0u :
                    static_cast<std::uint32_t>(text.size());
        record[13] = status;
        return parse_result;
    }
    const std::uint8_t* RecordTextData(
        const pal4::GameDataObjectRecord&) override {
        events.push_back("text_data");
        return null_text ? nullptr : text.data();
    }
    std::uint32_t FindKey(std::uint32_t, std::uint32_t,
                          std::uint32_t key) override {
        events.push_back("find");
        found_key = key;
        return position;
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataObjectRecord& record) override {
        events.push_back("insert_record:" + std::to_string(array) +
                         ":" + std::to_string(end));
        saved_record_hash = record[0];
    }
    void InsertKey(std::uint32_t array, std::uint32_t at,
                   std::uint32_t key, std::uint32_t index) override {
        events.push_back("insert_key:" + std::to_string(array) +
                         ":" + std::to_string(at));
        inserted_key = key;
        inserted_index = index;
    }
    void AssignRecord(std::uint32_t destination,
                      const pal4::GameDataObjectRecord& record) override {
        events.push_back("assign_record");
        assigned_destination = destination;
        saved_record_hash = record[0];
    }
    std::string DumpRow(std::uint32_t,
                        std::uint32_t) override {
        events.push_back("dump");
        return "row detail";
    }
};

Runtime MakeRuntime() {
    Runtime runtime;
    constexpr std::uint32_t table = 0x9000;
    runtime.words[table + 24] = 0x5000;
    runtime.words[table + 28] = 0x5000 + 2 * 60;
    runtime.words[table + 8] = 0x6000;
    runtime.words[table + 12] = 0x6010;
    runtime.words[table + 96] = 0x7000;
    runtime.words[table + 100] = 0x7000 + 3 * 56;
    runtime.position = 0x6010;
    return runtime;
}

void CheckInsertAndBinding() {
    Runtime runtime = MakeRuntime();
    runtime.collection_size = 1;
    pal4::LegacyCrc32 crc;
    Check(pal4::GameDataObjectTable::AddOrReplace(
              0x9000, 0xa000, runtime, crc) == 1 &&
              runtime.found_key == 0x340bc6d9u &&
              runtime.inserted_key == 0x340bc6d9u &&
              runtime.inserted_index == 3 &&
              runtime.saved_record_hash == 0x340bc6d9u,
          "new object row is hashed and inserted with its prior record count");
    Check(runtime.events == std::vector<std::string>{
              "size:36988", "log:beginBindField, table:36916",
              "assign_effects:36972:36884", "field:ID", "field:NAME",
              "field:POSITION", "field:TYPE", "bind:36972",
              "log:endBindField, table:36916", "initialize",
              "parse:36972:40960", "text_data", "find",
              "insert_record:36956:28840", "insert_key:36868:24592",
              "destroy_temporary"},
          "mismatched field count rebinds before parsing and inserting");
}

void CheckReplaceAndSignedInsert() {
    Runtime runtime = MakeRuntime();
    runtime.collection_size = 2;
    runtime.position = 0x6008;
    runtime.words[0x6008] = 0x340bc6d9u;
    runtime.words[0x600c] = 2;
    pal4::LegacyCrc32 crc;
    Check(pal4::GameDataObjectTable::AddOrReplace(
              0x9000, 0xa000, runtime, crc) == 1 &&
              runtime.assigned_destination == 0x7000 + 2 * 56 &&
              runtime.inserted_index == 0xffffffffu &&
              runtime.events.front() == "size:36988" &&
              runtime.events.back() == "destroy_temporary",
          "matching key updates the existing 56-byte record without rebind");
    for (const auto& event : runtime.events)
        Check(event.rfind("field:", 0) != 0,
              "matched field counts skip schema registration");

    Runtime negative_key = MakeRuntime();
    negative_key.collection_size = 2;
    negative_key.null_text = true;
    negative_key.position = 0x6000;
    negative_key.words[0x6000] = 0;
    pal4::LegacyCrc32 empty_crc;
    Check(pal4::GameDataObjectTable::AddOrReplace(
              0x9000, 0xa000, negative_key, empty_crc) == 1 &&
              negative_key.found_key == 0xffffffffu &&
              negative_key.inserted_key == 0xffffffffu &&
              negative_key.inserted_index == 3 &&
              negative_key.assigned_destination == 0,
          "signed key comparison inserts 0xffffffff before zero");
}

void CheckFailurePaths() {
    for (int failure = 0; failure < 2; ++failure) {
        Runtime runtime = MakeRuntime();
        runtime.collection_size = 2;
        runtime.parse_result = failure != 0;
        runtime.status = failure == 0 ? 2u : 4u;
        pal4::LegacyCrc32 crc;
        try {
            static_cast<void>(pal4::GameDataObjectTable::AddOrReplace(
                0x9000, 0xa000, runtime, crc));
            Check(false, "invalid object row must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      "ERROR <dbRecordBindTable< Record >::addRow >row detail" &&
                      runtime.events.back() == "destroy_temporary" &&
                      runtime.inserted_index == 0xffffffffu,
                  "invalid row dumps details and cleans temporary storage");
            if (failure == 0)
                for (const auto& event : runtime.events)
                    Check(event != "text_data",
                          "parse failure short-circuits hash calculation");
        }
    }
}

struct AppendRuntime final : pal4::IGameDataObjectAppendRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::array<std::uint8_t, 9> text{
        '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    std::uint32_t count = 2;
    std::uint32_t row_size = 2;
    std::uint32_t status = 2;
    std::uint32_t cleanup_result = 0xface;
    std::uint32_t appended_hash = 0;
    std::uint32_t append_array = 0;
    std::uint32_t append_end = 0;
    std::uint32_t fail_conversion = 0xffffffffu;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        events.push_back("size:" + std::to_string(address));
        return count;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        events.push_back("row_size:" + std::to_string(row));
        return row_size;
    }
    void InitializeRecord(pal4::GameDataObjectRecord& record) override {
        events.push_back("initialize");
        record[3] = text.size();
        record[13] = status;
    }
    std::uint32_t CleanupTemporaryRecord(
        pal4::GameDataObjectRecord&) override {
        events.push_back("cleanup");
        return cleanup_result;
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        events.push_back("property:" + std::to_string(index));
        return row + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataObjectRecord&,
                      std::uint32_t property) override {
        events.push_back("convert:" + std::to_string(field) +
                         ":" + std::to_string(property));
        if (property == fail_conversion)
            throw std::runtime_error("conversion failed");
    }
    const std::uint8_t* RecordTextData(
        const pal4::GameDataObjectRecord&) override {
        events.push_back("text_data");
        return text.data();
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataObjectRecord& record) override {
        events.push_back("append");
        append_array = array;
        append_end = end;
        appended_hash = record[0];
    }
    std::string DumpRow(std::uint32_t,
                        std::uint32_t) override {
        events.push_back("dump");
        return "row detail";
    }
};

AppendRuntime MakeAppendRuntime() {
    AppendRuntime runtime;
    runtime.words[0x9000 + 128] = 0x6000;
    runtime.words[0x6000] = 0x6100;
    runtime.words[0x6004] = 0x6200;
    runtime.words[0x9000 + 100] = 0x7000;
    return runtime;
}

void CheckAppend() {
    AppendRuntime runtime = MakeAppendRuntime();
    pal4::LegacyCrc32 crc;
    Check(pal4::GameDataObjectTable::Append(
              0x9000, 0xa000, runtime, crc) == runtime.cleanup_result &&
              runtime.append_array == 0x9000 + 92 &&
              runtime.append_end == 0x7000 &&
              runtime.appended_hash == 0x340bc6d9u,
          "append hashes one converted 56-byte row and forwards cleanup result");
    Check(runtime.events == std::vector<std::string>{
              "initialize", "size:36988", "row_size:40960",
              "size:36988", "property:0", "convert:24832:40960",
              "size:36988", "property:1", "convert:25088:40961",
              "size:36988", "text_data", "append", "cleanup"},
          "append converts fields in bound order before hashing");

    AppendRuntime empty = MakeAppendRuntime();
    empty.count = 0;
    empty.row_size = 0;
    pal4::LegacyCrc32 empty_crc;
    Check(pal4::GameDataObjectTable::Append(
              0x9000, 0xa000, empty, empty_crc) == empty.cleanup_result &&
              empty.appended_hash == 0x340bc6d9u,
          "zero-field row still validates and appends the initialized record");
    for (const auto& event : empty.events)
        Check(event.rfind("convert:", 0) != 0,
              "zero-field row does not invoke field conversion");
}

void CheckAppendFailures() {
    for (int failure = 0; failure < 3; ++failure) {
        AppendRuntime runtime = MakeAppendRuntime();
        if (failure == 0) runtime.row_size = 1;
        if (failure == 1) runtime.status = 4;
        if (failure == 2) runtime.fail_conversion = 0xa001;
        pal4::LegacyCrc32 crc;
        try {
            static_cast<void>(pal4::GameDataObjectTable::Append(
                0x9000, 0xa000, runtime, crc));
            Check(false, "invalid append must fail");
        } catch (const std::runtime_error& error) {
            const auto expected = failure == 2
                ? "conversion failed"
                : "ERROR <dbRecordBindTable< Record >::addRow >row detail";
            Check(std::string(error.what()) == expected &&
                      runtime.events.back() == "cleanup" &&
                      runtime.append_array == 0,
                  "append failure propagates and cleans temporary record");
            if (failure == 0)
                for (const auto& event : runtime.events)
                    Check(event.rfind("convert:", 0) != 0,
                          "field count mismatch skips conversion");
        }
    }
}

struct RefreshRuntime final : pal4::IGameDataObjectRefreshRuntime {
    std::vector<std::string> events;
    std::uint32_t result = 0x76543210;

    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.push_back("write:" + std::to_string(address) +
                         ":" + std::to_string(value));
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        events.push_back("refresh:" + std::to_string(table));
        return result;
    }
};

void CheckMarkAndRefresh() {
    RefreshRuntime runtime;
    Check(pal4::GameDataObjectTable::MarkAndRefresh(
              0x9000, runtime) == runtime.result &&
              runtime.events == std::vector<std::string>{
                  "write:36952:1", "refresh:36864"},
          "dirty flag is set before forwarding table refresh result");
}

}  // namespace

int main() {
    try {
        CheckInsertAndBinding();
        CheckReplaceAndSignedInsert();
        CheckFailurePaths();
        CheckAppend();
        CheckAppendFailures();
        CheckMarkAndRefresh();
        std::cout << "game_data_object_table_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
