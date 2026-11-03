#include "pal4/game_data_mission_table.h"

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

struct Runtime final : pal4::IGameDataMissionRowRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t collection_size = 2;
    std::uint32_t key = 11;
    std::uint32_t position = 0x3000;
    bool parse_result = true;

    void Log(const char* name, std::uint32_t address) {
        calls.push_back(std::string(name) + " " + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Log("read", address);
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Log("size", address);
        return collection_size;
    }
    void LogBind(std::string_view prefix, std::uint32_t table_name) override {
        calls.push_back(std::string(prefix) + " " +
                        std::to_string(table_name));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        calls.push_back("assign " + std::to_string(destination) + " " +
                        std::to_string(source));
    }
    void InitializeStructures(std::uint32_t binding) override {
        Log("initialize binding", binding);
    }
    void BindEffects(std::uint32_t binding) override {
        Log("bind", binding);
    }
    void InitializeRecord(pal4::GameDataMissionRecord& record) override {
        calls.push_back("initialize record");
        record.fill(0);
    }
    void DestroyRecord(pal4::GameDataMissionRecord&) override {
        calls.push_back("destroy record");
    }
    bool ParseBoundRow(std::uint32_t binding, std::uint32_t row,
                       pal4::GameDataMissionRecord& record) override {
        calls.push_back("parse " + std::to_string(binding) + " " +
                        std::to_string(row));
        record[0] = key;
        return parse_result;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t sought) override {
        calls.push_back("find " + std::to_string(begin) + " " +
                        std::to_string(end) + " " + std::to_string(sought));
        return position;
    }
    void CopyRecord(std::uint32_t destination,
                    const pal4::GameDataMissionRecord& record) override {
        Check(record[0] == key, "replacement must use parsed key");
        Log("copy", destination);
    }
    void InsertRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataMissionRecord& record) override {
        Check(record[0] == key, "insertion must use parsed record");
        calls.push_back("insert record " + std::to_string(array) + " " +
                        std::to_string(end));
    }
    void InsertKey(std::uint32_t array, std::uint32_t where,
                   std::uint32_t inserted_key,
                   std::uint32_t index) override {
        calls.push_back("insert key " + std::to_string(array) + " " +
                        std::to_string(where) + " " +
                        std::to_string(inserted_key) + " " +
                        std::to_string(index));
    }
    std::string DumpRow(std::uint32_t table, std::uint32_t row) override {
        calls.push_back("dump " + std::to_string(table) + " " +
                        std::to_string(row));
        return "Mission row detail";
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

Runtime BaseRuntime() {
    Runtime runtime;
    runtime.words[0x1018] = 0x2000;
    runtime.words[0x101c] = 0x2000 + 120;
    runtime.words[0x1008] = 0x3000;
    runtime.words[0x100c] = 0x3010;
    runtime.words[0x1060] = 0x4000;
    runtime.words[0x1064] = 0x4088;
    return runtime;
}

void CheckReplacementAndBinding() {
    auto runtime = BaseRuntime();
    runtime.collection_size = 1;
    runtime.words[0x3000] = 11;
    runtime.words[0x3004] = 1;
    Check(pal4::GameDataMissionTable::AddOrReplace(0x1000, 0x5000,
                                                   runtime) == 1,
          "Mission row replacement returns success");
    Check(At(runtime, "beginBindField, table: 4148") <
              At(runtime, "assign 4204 4116") &&
              At(runtime, "assign 4204 4116") <
                  At(runtime, "initialize binding 4204") &&
              At(runtime, "initialize binding 4204") <
                  At(runtime, "bind 4204") &&
              At(runtime, "bind 4204") <
                  At(runtime, "endBindField, table: 4148") &&
              At(runtime, "endBindField, table: 4148") <
                  At(runtime, "initialize record") &&
              At(runtime, "parse 4204 20480") <
                  At(runtime, "find 12288 12304 11") &&
              At(runtime, "find 12288 12304 11") <
                  At(runtime, "copy 16452") &&
              runtime.calls.back() == "destroy record",
          "mismatched field count binds before conversion and replaces index 1");
}

void CheckInsertion() {
    auto runtime = BaseRuntime();
    runtime.position = 0x3010;
    Check(pal4::GameDataMissionTable::AddOrReplace(0x1000, 0x5000,
                                                   runtime) == 1 &&
              At(runtime, "insert record 4188 16520") <
                  At(runtime, "insert key 4100 12304 11 2") &&
              runtime.calls.back() == "destroy record",
          "missing key inserts record before sorted key with 68-byte index");
    Check(std::find_if(runtime.calls.begin(), runtime.calls.end(),
                       [](const std::string& call) {
                           return call.starts_with("beginBindField");
                       }) == runtime.calls.end(),
          "matching field count skips rebinding");

    auto signed_runtime = BaseRuntime();
    signed_runtime.key = static_cast<std::uint32_t>(-2);
    signed_runtime.words[0x3000] = 1;
    Check(pal4::GameDataMissionTable::AddOrReplace(
              0x1000, 0x5000, signed_runtime) == 1 &&
              At(signed_runtime, "insert record 4188 16520") <
                  At(signed_runtime,
                     "insert key 4100 12288 4294967294 2"),
          "signed key less than candidate is inserted rather than replaced");
}

void CheckError() {
    auto runtime = BaseRuntime();
    runtime.parse_result = false;
    try {
        (void)pal4::GameDataMissionTable::AddOrReplace(0x1000, 0x5000,
                                                       runtime);
        throw std::runtime_error("expected row failure");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow >Mission row detail",
              "row error includes original prefix and dumped row");
    }
    Check(At(runtime, "parse 4204 20480") <
              At(runtime, "dump 4096 20480") &&
              runtime.calls.back() == "destroy record",
          "failed row cleans prepared text fields before propagating error");
}

}  // namespace

int main() {
    try {
        CheckReplacementAndBinding();
        CheckInsertion();
        CheckError();
        std::cout << "game_data_mission_row_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
