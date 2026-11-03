#include "pal4/utilities_string_id_table.h"

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

struct Runtime final : pal4::IUtilitiesStringIdRowRuntime,
                       pal4::IUtilitiesStringIdAppendRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::int32_t bound_count = 2;
    std::uint32_t row_size = 2;
    std::uint32_t parsed_key = 7;
    std::uint32_t parsed_value = 70;
    bool fail_convert = false;
    bool fail_insert = false;

    Runtime() {
        words[0x1018] = 0x2000;
        words[0x101c] = 0x203c;
        words[0x1080] = 0x4000;
        words[0x4000] = 0x6000;
        words[0x4004] = 0x6010;
        words[0x1060] = 0x5000;
        words[0x1064] = 0x5010;
        words[0x1008] = 0x3000;
        words[0x100c] = 0x3010;
        words[0x3000] = 3;
        words[0x3004] = 0;
        words[0x3008] = 9;
        words[0x300c] = 1;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back({"read", address});
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        calls.push_back({"row_size", row});
        return row_size;
    }
    std::int32_t CollectionSize(std::uint32_t collection) override {
        calls.push_back({"count", collection});
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
    void RegisterStringIdFields(std::uint32_t binding) override {
        calls.push_back({"register_string_id", binding});
    }
    void BindEffects(std::uint32_t binding) override {
        calls.push_back({"bind", binding});
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        calls.push_back({"property", row, index});
        return 0x7000 + 4 * index;
    }
    void ConvertField(std::uint32_t binding,
                      pal4::UtilitiesStringIdRecord& record,
                      std::uint32_t property) override {
        calls.push_back({"convert", binding, property});
        if (fail_convert) throw std::runtime_error("conversion failed");
        record[(binding - 0x6000) / 16] =
            binding == 0x6000 ? parsed_key : parsed_value;
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::UtilitiesStringIdRecord& record) override {
        Check(record[0] == parsed_key && record[1] == parsed_value,
              "both converted integers reach insertion");
        calls.push_back({"insert_record", array, end, record[0]});
    }
    void InsertKey(std::uint32_t array, std::uint32_t position,
                   std::uint32_t key, std::uint32_t index) override {
        Check(key == parsed_key, "key matches converted ID");
        calls.push_back({"insert_key", array, position, index});
        if (fail_insert) throw std::runtime_error("insert failed");
    }
    std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const pal4::UtilitiesStringIdRecord& record) override {
        Check(record[0] == parsed_key && record[1] == parsed_value,
              "both converted integers reach append");
        calls.push_back({"append", array, end, record[0]});
        return 0x78;
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        calls.push_back({"dump", table, row});
        return " row=42";
    }
};

std::size_t Position(const Runtime& runtime, std::string_view name) {
    const auto found = std::find_if(runtime.calls.begin(),
                                    runtime.calls.end(),
                                    [name](const Call& call) {
                                        return call.name == name;
                                    });
    Check(found != runtime.calls.end(), "expected native call occurred");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

bool HasCall(const Runtime& runtime, std::string_view name) {
    return std::any_of(runtime.calls.begin(), runtime.calls.end(),
                       [name](const Call& call) {
                           return call.name == name;
                       });
}

void CheckInsertAndRebind() {
    Runtime runtime;
    Check(pal4::UtilitiesStringIdTable::AddOrReplace(
              0x1000, 0x8000, runtime) == 1 &&
              runtime.calls[runtime.calls.size() - 2] ==
                  Call{"insert_record", 0x105c, 0x5010, 7} &&
              runtime.calls.back() ==
                  Call{"insert_key", 0x1004, 0x3008, 2},
          "new ID inserts at physical position two and sorted key position");
    Check(Position(runtime, "begin_bind") <
              Position(runtime, "assign") &&
              Position(runtime, "assign") <
                  Position(runtime, "register_string_id") &&
              Position(runtime, "register_string_id") <
                  Position(runtime, "bind") &&
              Position(runtime, "bind") <
                  Position(runtime, "end_bind") &&
              Position(runtime, "end_bind") <
                  Position(runtime, "row_size") &&
              Position(runtime, "row_size") <
                  Position(runtime, "convert") &&
              Position(runtime, "convert") <
                  Position(runtime, "insert_record"),
          "field rebinding precedes width check and conversion");

    Runtime already_bound;
    already_bound.words[0x101c] = 0x2078;
    Check(pal4::UtilitiesStringIdTable::AddOrReplace(
              0x1000, 0x8000, already_bound) == 1 &&
              !HasCall(already_bound, "begin_bind"),
          "matching source and bound field counts skip rebinding");
}

void CheckReplaceAndSignedKey() {
    Runtime existing;
    existing.parsed_key = 9;
    existing.parsed_value = 99;
    Check(pal4::UtilitiesStringIdTable::AddOrReplace(
              0x1000, 0x8000, existing) == 1 &&
              existing.words[0x5008] == 9 &&
              existing.words[0x500c] == 99 &&
              !HasCall(existing, "insert_record") &&
              !HasCall(existing, "insert_key") &&
              existing.calls[existing.calls.size() - 2] ==
                  Call{"write", 0x5008, 9} &&
              existing.calls.back() == Call{"write", 0x500c, 99},
          "existing ID replaces two POD words at indexed row");

    Runtime negative;
    negative.parsed_key = static_cast<std::uint32_t>(-2);
    Check(pal4::UtilitiesStringIdTable::AddOrReplace(
              0x1000, 0x8000, negative) == 1 &&
              negative.calls.back() ==
                  Call{"insert_key", 0x1004, 0x3000, 2},
          "signed negative key sorts before positive IDs");

    Runtime duplicate;
    duplicate.words[0x3000] = 9;
    duplicate.words[0x3004] = 0;
    duplicate.parsed_key = 9;
    Check(pal4::UtilitiesStringIdTable::AddOrReplace(
              0x1000, 0x8000, duplicate) == 1 &&
              duplicate.calls[duplicate.calls.size() - 2] ==
                  Call{"write", 0x5000, 9},
          "lower bound selects first equal-key index");
}

void CheckAddFailures() {
    Runtime mismatch;
    mismatch.row_size = 1;
    try {
        static_cast<void>(pal4::UtilitiesStringIdTable::AddOrReplace(
            0x1000, 0x8000, mismatch));
        Check(false, "row width mismatch must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow > row=42" &&
                  mismatch.calls.back() == Call{"dump", 0x1000, 0x8000} &&
                  !HasCall(mismatch, "convert"),
              "width mismatch includes row details before conversion");
    }

    Runtime interrupted;
    interrupted.fail_convert = true;
    try {
        static_cast<void>(pal4::UtilitiesStringIdTable::AddOrReplace(
            0x1000, 0x8000, interrupted));
        Check(false, "conversion failure must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "conversion failed" &&
                  !HasCall(interrupted, "insert_record"),
              "conversion failure stops before storage writes");
    }

    Runtime insert_failure;
    insert_failure.fail_insert = true;
    try {
        static_cast<void>(pal4::UtilitiesStringIdTable::AddOrReplace(
            0x1000, 0x8000, insert_failure));
        Check(false, "index insertion failure must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "insert failed" &&
                  HasCall(insert_failure, "insert_record"),
              "record insertion precedes index insertion");
    }
}

void CheckAppend() {
    Runtime runtime;
    Check(pal4::UtilitiesStringIdTable::Append(
              0x1000, 0x8000, runtime) == 0x78 &&
              runtime.calls.back() ==
                  Call{"append", 0x105c, 0x5010, 7} &&
              !HasCall(runtime, "begin_bind") &&
              Position(runtime, "row_size") <
                  Position(runtime, "convert") &&
              Position(runtime, "convert") <
                  Position(runtime, "append"),
          "append converts two integers and forwards storage result");

    Runtime mismatch;
    mismatch.row_size = 1;
    try {
        static_cast<void>(pal4::UtilitiesStringIdTable::Append(
            0x1000, 0x8000, mismatch));
        Check(false, "append width mismatch must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow > row=42" &&
                  mismatch.calls.back() == Call{"dump", 0x1000, 0x8000} &&
                  !HasCall(mismatch, "append"),
              "append width mismatch reports row and does not write");
    }
}

struct RefreshRuntime final : pal4::IUtilitiesStringIdRefreshRuntime {
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
    Check(pal4::UtilitiesStringIdTable::MarkAndRefresh(
              0x1000, runtime) == 0x12345678 &&
              runtime.calls == std::vector<Call>{{"mark", 0x1058, 1},
                                                 {"refresh", 0x1000}},
          "dirty byte precedes refresh and forwards its result");
}

}  // namespace

int main() {
    try {
        CheckInsertAndRebind();
        CheckReplaceAndSignedKey();
        CheckAddFailures();
        CheckAppend();
        CheckMarkAndRefresh();
        std::cout << "utilities_string_id_row_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
