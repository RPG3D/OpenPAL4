#include "pal4/skill_data_table.h"

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
    std::string kind;
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    std::uint32_t third = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::ISkillDataRowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::int32_t collection_size = 1;
    std::uint32_t position = 0x3008;
    std::uint32_t key = 5;
    std::uint32_t type = 3;
    std::uint32_t state = 14;
    bool parse_result = true;
    bool fail_insert_key = false;

    Runtime() {
        words[0x1000 + 24] = 0x2000;
        words[0x1000 + 28] = 0x2000 + 120;
        words[0x1000 + 8] = 0x3000;
        words[0x1000 + 12] = 0x3010;
        words[0x3008] = 8;
        words[0x3008 + 4] = 3;
        words[0x1000 + 96] = 0x4000;
        words[0x1000 + 100] = 0x4000 + 264;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::int32_t CollectionSize(std::uint32_t address) override {
        calls.push_back({"count", address});
        return collection_size;
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
        calls.push_back({"bind_effects", binding});
    }
    void InitializeRecord(pal4::SkillDataRecord& record) override {
        record.fill(0);
        calls.push_back({"initialize_record"});
    }
    void DestroyTemporaryRecord(pal4::SkillDataRecord&) override {
        calls.push_back({"destroy_record"});
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::SkillDataRecord& record) override {
        calls.push_back({"parse", binding, row});
        record[0] = key;
        record[1] = type;
        record[2] = state;
        return parse_result;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t value) override {
        calls.push_back({"find", begin, end, value});
        return position;
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::SkillDataRecord& record) override {
        calls.push_back({"insert_record", array, end, record[0]});
    }
    void InsertKey(std::uint32_t array, std::uint32_t entry,
                   std::uint32_t value, std::uint32_t index) override {
        calls.push_back({"insert_key", array, entry, index});
        if (value != key) throw std::runtime_error("unexpected key");
        if (fail_insert_key) throw std::runtime_error("insert failed");
    }
    void AssignRecord(std::uint32_t destination,
                      const pal4::SkillDataRecord& record) override {
        calls.push_back({"assign_record", destination, record[0]});
    }
    std::string DumpRow(std::uint32_t table,
                        std::uint32_t row) override {
        calls.push_back({"dump", table, row});
        return " row";
    }
};

bool HasCall(const Runtime& runtime, const std::string& kind) {
    for (const auto& call : runtime.calls)
        if (call.kind == kind) return true;
    return false;
}

void CheckInsertAndReplace() {
    Runtime runtime;
    Check(pal4::SkillDataTable::AddOrReplace(0x1000, 0x5000, runtime) == 1 &&
              runtime.calls == std::vector<Call>{
                                   {"count", 0x1000 + 124},
                                   {"begin_bind", 0x1000 + 52},
                                   {"assign_effects", 0x1000 + 108,
                                    0x1000 + 20},
                                   {"properties", 0x1000 + 108},
                                   {"bind_effects", 0x1000 + 108},
                                   {"end_bind", 0x1000 + 52},
                                   {"initialize_record"},
                                   {"parse", 0x1000 + 108, 0x5000},
                                   {"find", 0x3000, 0x3010, 5},
                                   {"insert_record", 0x1000 + 92,
                                    0x4000 + 264, 5},
                                   {"insert_key", 0x1000 + 4,
                                    0x3008, 2},
                                   {"destroy_record"}},
          "mismatch rebinds then inserts 132-byte record and signed key");

    Runtime replaced;
    replaced.collection_size = 2;
    replaced.words[0x3008] = 5;
    Check(pal4::SkillDataTable::AddOrReplace(
              0x1000, 0x5000, replaced) == 1 &&
              !HasCall(replaced, "begin_bind") &&
              replaced.calls[3] == Call{"find", 0x3000, 0x3010, 5} &&
              replaced.calls[4] ==
                  Call{"assign_record", 0x4000 + 3 * 132, 5} &&
              replaced.calls.back() == Call{"destroy_record"},
          "matching bindings replace existing physical record");

    Runtime empty;
    empty.words[0x1000 + 96] = 0;
    empty.words[0x1000 + 100] = 0;
    empty.position = 0x3010;
    Check(pal4::SkillDataTable::AddOrReplace(0x1000, 0x5000, empty) == 1 &&
              empty.calls[empty.calls.size() - 2] ==
                  Call{"insert_key", 0x1000 + 4, 0x3010, 0},
          "empty storage uses zero physical position");
}

void CheckInvalidAndFailure() {
    for (int case_index = 0; case_index < 3; ++case_index) {
        Runtime runtime;
        runtime.collection_size = 2;
        if (case_index == 0) runtime.parse_result = false;
        if (case_index == 1) runtime.type = 4;
        if (case_index == 2) runtime.state = 15;
        try {
            static_cast<void>(pal4::SkillDataTable::AddOrReplace(
                0x1000, 0x5000, runtime));
            Check(false, "invalid row must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      "ERROR <dbRecordBindTable< Record >::addRow > row" &&
                      HasCall(runtime, "dump") &&
                      !HasCall(runtime, "find") &&
                      runtime.calls.back() == Call{"destroy_record"},
                  "invalid row dumps and cleans up before index access");
        }
    }
    Runtime interrupted;
    interrupted.collection_size = 2;
    interrupted.fail_insert_key = true;
    try {
        static_cast<void>(pal4::SkillDataTable::AddOrReplace(
            0x1000, 0x5000, interrupted));
        Check(false, "index insertion failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "insert failed" &&
                  HasCall(interrupted, "insert_record") &&
                  interrupted.calls.back() == Call{"destroy_record"},
              "temporary Skill record is cleaned after index failure");
    }
}

}  // namespace

int main() {
    try {
        CheckInsertAndReplace();
        CheckInvalidAndFailure();
        std::cout << "skill_data_table_row_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
