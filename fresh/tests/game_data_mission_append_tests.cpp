#include "pal4/game_data_mission_table.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataMissionAppendRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t row_size = 2;
    std::uint32_t bound_size = 2;
    std::uint32_t release_result = 0x7777;

    void Log(const char* name, std::uint32_t value) {
        calls.push_back(std::string(name) + " " + std::to_string(value));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Log("read", address);
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Log("size", address);
        return bound_size;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Log("row size", row);
        return row_size;
    }
    void InitializeRecord(pal4::GameDataMissionRecord& record) override {
        calls.emplace_back("initialize record");
        record.fill(0);
    }
    std::uint32_t DestroyRecord(
        pal4::GameDataMissionRecord& record) override {
        Check(record[0] == bound_size || row_size != bound_size,
              "record conversion runs before text cleanup");
        calls.emplace_back("destroy record");
        return release_result;
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        calls.push_back("property " + std::to_string(row) + " " +
                        std::to_string(index));
        return 0x5000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataMissionRecord& record,
                      std::uint32_t property) override {
        calls.push_back("convert " + std::to_string(field) + " " +
                        std::to_string(property));
        ++record[0];
    }
    void AppendRecord(std::uint32_t array, std::uint32_t end,
                      const pal4::GameDataMissionRecord& record) override {
        Check(record[0] == bound_size,
              "all bound properties must be converted before append");
        calls.push_back("append " + std::to_string(array) + " " +
                        std::to_string(end));
    }
    std::string DumpRow(std::uint32_t table, std::uint32_t row) override {
        calls.push_back("dump " + std::to_string(table) + " " +
                        std::to_string(row));
        return "Mission append detail";
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

void CheckSuccess() {
    Runtime runtime;
    runtime.words[0x1080] = 0x3000;
    runtime.words[0x3000] = 0x4000;
    runtime.words[0x3004] = 0x4060;
    runtime.words[0x1064] = 0x5000;
    Check(pal4::GameDataMissionTable::Append(0x1000, 0x2000, runtime) ==
              runtime.release_result,
          "append forwards final temporary text release result");
    Check(runtime.calls[0] == "initialize record", "record initializes first");
    Check(At(runtime, "row size 8192") < At(runtime, "size 4220"),
          "row size is checked before bound size");
    Check(At(runtime, "property 8192 0") <
              At(runtime, "convert 16384 20480"),
          "first property is read before conversion");
    Check(At(runtime, "convert 16384 20480") <
              At(runtime, "property 8192 1"),
          "fields are converted in index order");
    Check(At(runtime, "convert 16480 20481") <
              At(runtime, "append 4188 20480"),
          "both conversions precede append");
    Check(runtime.calls.back() == "destroy record",
          "record text is released after append");

    Runtime empty;
    empty.bound_size = 0;
    empty.row_size = 0;
    empty.words[0x1064] = 0x5000;
    Check(pal4::GameDataMissionTable::Append(0x1000, 0x2000, empty) ==
              empty.release_result &&
              At(empty, "append 4188 20480") <
                  At(empty, "destroy record"),
          "zero-field rows still append one empty record");
}

void CheckMismatch() {
    Runtime runtime;
    runtime.row_size = 1;
    try {
        (void)pal4::GameDataMissionTable::Append(0x1000, 0x2000,
                                                  runtime);
        throw std::runtime_error("expected row failure");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "ERROR <dbRecordBindTable< Record >::addRow >Mission append detail",
              "field count mismatch throws with dumped row");
    }
    Check(At(runtime, "dump 4096 8192") < At(runtime, "destroy record") &&
              runtime.calls.back() == "destroy record",
          "failed append releases temporary record after formatting error");
}

}  // namespace

int main() {
    try {
        CheckSuccess();
        CheckMismatch();
        std::cout << "game_data_mission_append_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
