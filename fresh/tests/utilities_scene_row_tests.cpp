#include "pal4/utilities_scene_table.h"

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

struct Call {
    std::string name;
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    std::uint32_t third = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IUtilitiesSceneRowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::int32_t bound_count = 1;
    std::uint32_t parsed_key = 7;
    bool parse_ok = true;
    bool fail_insert = false;

    Runtime() {
        words[0x1018] = 0x2000;
        words[0x101c] = 0x2078;
        words[0x1008] = 0x3000;
        words[0x100c] = 0x3010;
        words[0x3000] = 3;
        words[0x3004] = 0;
        words[0x3008] = 9;
        words[0x300c] = 2;
        words[0x1060] = 0x4000;
        words[0x1064] = 0x40b4;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::int32_t CollectionSize(std::uint32_t collection) override {
        calls.push_back({"size", collection});
        return bound_count;
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t name) override {
        calls.push_back({prefix == "beginBindField, table:"
                             ? "begin_bind" : "end_bind", name});
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        calls.push_back({"assign", destination, source});
    }
    void RegisterSceneFields(std::uint32_t binding) override {
        calls.push_back({"register_scene", binding});
    }
    void BindEffects(std::uint32_t binding) override {
        calls.push_back({"bind", binding});
    }
    void InitializeRecord(pal4::UtilitiesSceneRecord& record) override {
        record.fill(0);
        calls.push_back({"initialize"});
    }
    void DestroyTemporaryRecord(
        pal4::UtilitiesSceneRecord&) override {
        calls.push_back({"destroy_temporary"});
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::UtilitiesSceneRecord& record) override {
        calls.push_back({"parse", binding, row});
        record[0] = parsed_key;
        record[1] = 0x1111;
        record[5] = 0x5555;
        record[9] = 0x9999;
        return parse_ok;
    }
    void UppercaseText(pal4::UtilitiesSceneRecord& record,
                       std::uint32_t text_offset) override {
        calls.push_back({"uppercase", text_offset});
        ++record[text_offset / 4];
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::UtilitiesSceneRecord& record) override {
        Check(record[1] == 0x1112 && record[5] == 0x5556 &&
                  record[9] == 0x9999,
              "only first two Scene text fields were uppercased");
        calls.push_back({"insert_record", array, end, record[0]});
    }
    void InsertKey(std::uint32_t array, std::uint32_t position,
                   std::uint32_t key, std::uint32_t index) override {
        Check(key == parsed_key, "index key matches parsed record");
        calls.push_back({"insert_key", array, position, index});
        if (fail_insert) throw std::runtime_error("insert failed");
    }
    void AssignRecord(std::uint32_t destination,
                      const pal4::UtilitiesSceneRecord& record) override {
        Check(record[1] == 0x1112 && record[5] == 0x5556 &&
                  record[9] == 0x9999,
              "replacement sees normalized first two fields");
        calls.push_back({"assign_record", destination, record[0]});
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        calls.push_back({"dump", table, row});
        return " row=42";
    }
};

bool HasCall(const Runtime& runtime, std::string_view name) {
    return std::any_of(runtime.calls.begin(), runtime.calls.end(),
                       [name](const Call& call) {
                           return call.name == name;
                       });
}

void CheckInsertAndRebind() {
    Runtime runtime;
    Check(pal4::UtilitiesSceneTable::AddOrReplace(
              0x1000, 0x5000, runtime) == 1 &&
              runtime.calls == std::vector<Call>{
                  {"size", 0x107c}, {"begin_bind", 0x1034},
                  {"assign", 0x106c, 0x1014},
                  {"register_scene", 0x106c}, {"bind", 0x106c},
                  {"end_bind", 0x1034}, {"initialize"},
                  {"parse", 0x106c, 0x5000}, {"uppercase", 4},
                  {"uppercase", 20},
                  {"insert_record", 0x105c, 0x40b4, 7},
                  {"insert_key", 0x1004, 0x3008, 3},
                  {"destroy_temporary"}},
          "Scene row rebinds, parses, normalizes and inserts in IDA order");

    Runtime empty;
    empty.words[0x1018] = 0;
    empty.words[0x100c] = 0x3000;
    empty.words[0x1060] = 0;
    empty.words[0x1064] = 0;
    empty.bound_count = 0;
    Check(pal4::UtilitiesSceneTable::AddOrReplace(
              0x1000, 0x5000, empty) == 1 &&
              !HasCall(empty, "begin_bind") &&
              empty.calls[empty.calls.size() - 2] ==
                  Call{"insert_key", 0x1004, 0x3000, 0},
          "empty field and record arrays insert at physical index zero");
}

void CheckReplaceAndSignedKey() {
    Runtime existing;
    existing.bound_count = 2;
    existing.parsed_key = 9;
    Check(pal4::UtilitiesSceneTable::AddOrReplace(
              0x1000, 0x5000, existing) == 1 &&
              existing.calls[existing.calls.size() - 2] ==
                  Call{"assign_record", 0x4078, 9} &&
              !HasCall(existing, "insert_record"),
          "existing key selects 60-byte physical record");

    Runtime negative;
    negative.bound_count = 2;
    negative.parsed_key = static_cast<std::uint32_t>(-2);
    Check(pal4::UtilitiesSceneTable::AddOrReplace(
              0x1000, 0x5000, negative) == 1 &&
              negative.calls[negative.calls.size() - 2] ==
                  Call{"insert_key", 0x1004, 0x3000, 3},
          "signed negative key sorts before positive keys");

    Runtime duplicate;
    duplicate.bound_count = 2;
    duplicate.words[0x3000] = 9;
    duplicate.words[0x3004] = 1;
    duplicate.parsed_key = 9;
    Check(pal4::UtilitiesSceneTable::AddOrReplace(
              0x1000, 0x5000, duplicate) == 1 &&
              duplicate.calls[duplicate.calls.size() - 2] ==
                  Call{"assign_record", 0x403c, 9},
          "lower bound resolves first duplicate index entry");
}

void CheckFailureCleanup() {
    Runtime invalid;
    invalid.bound_count = 2;
    invalid.parse_ok = false;
    try {
        static_cast<void>(pal4::UtilitiesSceneTable::AddOrReplace(
            0x1000, 0x5000, invalid));
        Check(false, "parse failure must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow > row=42" &&
                  invalid.calls[invalid.calls.size() - 2] ==
                      Call{"dump", 0x1000, 0x5000} &&
                  invalid.calls.back() == Call{"destroy_temporary"} &&
                  !HasCall(invalid, "uppercase") &&
                  !HasCall(invalid, "insert_record"),
              "parse error reports row and cleans temporary texts");
    }

    Runtime interrupted;
    interrupted.bound_count = 2;
    interrupted.fail_insert = true;
    try {
        static_cast<void>(pal4::UtilitiesSceneTable::AddOrReplace(
            0x1000, 0x5000, interrupted));
        Check(false, "key insertion failure must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "insert failed" &&
                  interrupted.calls.back() == Call{"destroy_temporary"},
              "insertion failure cleans temporary row");
    }
}

struct AppendRuntime final : pal4::IUtilitiesSceneAppendRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::uint32_t row_size = 2;
    std::uint32_t bound_count = 2;
    bool fail_convert = false;

    AppendRuntime() {
        words[0x1080] = 0x3000;
        words[0x3000] = 0x4000;
        words[0x3004] = 0x4010;
        words[0x1064] = 0x5000;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back({"read", address});
        return words.at(address);
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        calls.push_back({"row_size", row});
        return row_size;
    }
    std::uint32_t CollectionSize(std::uint32_t collection) override {
        calls.push_back({"count", collection});
        return bound_count;
    }
    void InitializeRecord(pal4::UtilitiesSceneRecord& record) override {
        record.fill(0);
        calls.push_back({"initialize"});
    }
    std::uint32_t CleanupTemporaryRecord(
        pal4::UtilitiesSceneRecord&) override {
        calls.push_back({"cleanup"});
        return 0x77;
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        calls.push_back({"property", row, index});
        return 0x6000 + 4 * index;
    }
    void ConvertField(std::uint32_t binding,
                      pal4::UtilitiesSceneRecord& record,
                      std::uint32_t property) override {
        calls.push_back({"convert", binding, property});
        if (fail_convert) throw std::runtime_error("conversion failed");
        record[(binding - 0x4000) / 16] = property;
    }
    void UppercaseText(pal4::UtilitiesSceneRecord& record,
                       std::uint32_t text_offset) override {
        calls.push_back({"uppercase", text_offset});
        ++record[text_offset / 4];
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::UtilitiesSceneRecord& record) override {
        calls.push_back({"append", array, end, record[0]});
        Check(record[1] == (bound_count > 1 ? 0x6005u : 1u) &&
                  record[5] == 1,
              "both text slots normalize before append");
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        calls.push_back({"dump", table, row});
        return " row=42";
    }
};

void CheckAppend() {
    AppendRuntime runtime;
    Check(pal4::UtilitiesSceneTable::Append(
              0x1000, 0x7000, runtime) == 0x77 &&
              runtime.calls == std::vector<Call>{
                  {"initialize"}, {"row_size", 0x7000},
                  {"count", 0x107c}, {"count", 0x107c},
                  {"read", 0x1080}, {"read", 0x3000},
                  {"property", 0x7000, 0},
                  {"convert", 0x4000, 0x6000},
                  {"count", 0x107c}, {"read", 0x1080},
                  {"read", 0x3004}, {"property", 0x7000, 1},
                  {"convert", 0x4010, 0x6004}, {"count", 0x107c},
                  {"uppercase", 4}, {"uppercase", 20},
                  {"read", 0x1064}, {"append", 0x105c, 0x5000, 0x6000},
                  {"cleanup"}},
          "append converts bound fields, uppercases and returns cleanup");

    AppendRuntime empty;
    empty.row_size = 0;
    empty.bound_count = 0;
    Check(pal4::UtilitiesSceneTable::Append(
              0x1000, 0x7000, empty) == 0x77 &&
              empty.calls == std::vector<Call>{
                  {"initialize"}, {"row_size", 0x7000},
                  {"count", 0x107c}, {"count", 0x107c},
                  {"uppercase", 4}, {"uppercase", 20},
                  {"read", 0x1064}, {"append", 0x105c, 0x5000, 0},
                  {"cleanup"}},
          "zero-field row still normalizes and appends");
}

void CheckAppendFailures() {
    AppendRuntime mismatch;
    mismatch.row_size = 1;
    try {
        static_cast<void>(pal4::UtilitiesSceneTable::Append(
            0x1000, 0x7000, mismatch));
        Check(false, "row width mismatch must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow > row=42" &&
                  mismatch.calls[mismatch.calls.size() - 2] ==
                      Call{"dump", 0x1000, 0x7000} &&
                  mismatch.calls.back() == Call{"cleanup"},
              "width mismatch reports row and cleans temporary record");
    }

    AppendRuntime interrupted;
    interrupted.fail_convert = true;
    try {
        static_cast<void>(pal4::UtilitiesSceneTable::Append(
            0x1000, 0x7000, interrupted));
        Check(false, "conversion failure must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "conversion failed" &&
                  interrupted.calls.back() == Call{"cleanup"} &&
                  !std::any_of(interrupted.calls.begin(),
                               interrupted.calls.end(),
                               [](const Call& call) {
                                   return call.name == "append";
                               }),
              "conversion failure does not append and cleans temporary row");
    }
}

struct RefreshRuntime final : pal4::IUtilitiesSceneRefreshRuntime {
    std::vector<Call> calls;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back({"mark", address, value});
    }
    std::uint32_t Refresh(std::uint32_t object) override {
        calls.push_back({"refresh", object});
        return 0x12345678;
    }
};

void CheckMarkAndRefresh() {
    RefreshRuntime runtime;
    Check(pal4::UtilitiesSceneTable::MarkAndRefresh(
              0x1000, runtime) == 0x12345678 &&
              runtime.calls == std::vector<Call>{{"mark", 0x1058, 1},
                                                 {"refresh", 0x1000}},
          "dirty byte precedes refresh and its result is forwarded");
}

}  // namespace

int main() {
    try {
        CheckInsertAndRebind();
        CheckReplaceAndSignedKey();
        CheckFailureCleanup();
        CheckAppend();
        CheckAppendFailures();
        CheckMarkAndRefresh();
        std::cout << "utilities_scene_row_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
