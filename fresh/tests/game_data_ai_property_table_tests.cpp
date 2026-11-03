#include "pal4/game_data_ai_property_table.h"

#include <algorithm>
#include <bit>
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

struct Runtime final : pal4::IGameDataAiPropertyTableRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t collection_size = 2;
    std::uint32_t row_size = 2;
    std::uint32_t found = 0x5000;
    std::uint32_t first_index = 0;
    float first_weight = 1.0f;
    float second_weight = 1.0f;
    pal4::GameDataAiPropertyRecord copied{};

    Runtime() {
        words[0x1018] = 0x2000;
        words[0x101c] = 0x2078;
        words[0x1080] = 0x3000;
        words[0x1084] = 0x3008;
        words[0x3000] = 0x4000;
        words[0x3004] = 0x4100;
        words[0x1008] = 0x5000;
        words[0x100c] = 0x5010;
        words[0x5000] = 12;
        words[0x5004] = 1;
        words[0x1060] = 0x6000;
        words[0x1064] = 0x6088;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t CollectionSize(std::uint32_t address) override {
        Check(address == 0x107c, "collection address");
        return collection_size;
    }
    std::uint32_t RowSize(std::uint32_t row) override {
        Check(row == 0x9000, "row address");
        calls.emplace_back("size");
        return row_size;
    }
    void LogBind(std::string_view prefix, std::uint32_t name) override {
        Check(name == 0x1034, "table name address");
        calls.push_back(std::string(prefix));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x106c && source == 0x1014,
              "effect assignment arguments");
        calls.emplace_back("assign");
    }
    void InitializeStructures(std::uint32_t object) override {
        Check(object == 0x106c, "property structure address");
        calls.emplace_back("initialize property");
    }
    void BindEffects(std::uint32_t object) override {
        Check(object == 0x106c, "effect binding address");
        calls.emplace_back("bind");
    }
    std::uint32_t PropertyAt(std::uint32_t row,
                             std::uint32_t index) override {
        Check(row == 0x9000 && index < 2, "property index");
        calls.push_back("property " + std::to_string(index));
        return 0xa000 + index;
    }
    void ConvertField(std::uint32_t field,
                      pal4::GameDataAiPropertyRecord& record,
                      std::uint32_t property) override {
        Check((field == 0x4000 && property == 0xa000) ||
                  (field == 0x4100 && property == 0xa001),
              "field and property pairing");
        calls.push_back("convert " + std::to_string(field));
        record[0] = 12;
        record[2] = first_index;
        record[3] = 0;
        record[6] = std::bit_cast<std::uint32_t>(first_weight);
        record[15] = std::bit_cast<std::uint32_t>(second_weight);
        record[16] = 0x1234;
    }
    std::uint32_t FindKey(std::uint32_t begin, std::uint32_t end,
                          std::uint32_t key) override {
        Check(begin == 0x5000 && end == 0x5010 && key == 12,
              "lookup range and key");
        calls.emplace_back("find");
        return found;
    }
    void CopyRecord(std::uint32_t destination,
                    const pal4::GameDataAiPropertyRecord& record) override {
        Check(destination == 0x6044, "indexed 68-byte record address");
        copied = record;
        calls.emplace_back("copy");
    }
    void InsertRecord(std::uint32_t array, std::uint32_t position,
                      const pal4::GameDataAiPropertyRecord& record) override {
        Check(array == 0x105c && position == 0x6088 &&
                  record[16] == 0x1234,
              "68-byte record insert arguments");
        calls.emplace_back("insert record");
    }
    void InsertKey(std::uint32_t array, std::uint32_t position,
                   std::uint32_t key, std::uint32_t index) override {
        Check(array == 0x1004 && position == found && key == 12 && index == 2,
              "key insertion stores the old record count");
        calls.emplace_back("insert key");
    }
    std::uint32_t AppendRecord(
        std::uint32_t array, std::uint32_t end,
        const pal4::GameDataAiPropertyRecord& record) override {
        Check(array == 0x105c && end == 0x6088 && record[16] == 0x1234,
              "append arguments");
        calls.emplace_back("append");
        return 0x7777;
    }
    std::string DumpRow(std::uint32_t table, std::uint32_t row) override {
        Check(table == 0x1000 && row == 0x9000, "dump context");
        calls.emplace_back("dump");
        return ": property row";
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == 0x1058 && value == 1, "property refresh flag");
        calls.emplace_back("flag 1");
    }
    std::uint32_t Refresh(std::uint32_t table) override {
        Check(table == 0x1000, "property refresh owner");
        calls.emplace_back("refresh");
        return 0x1234;
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected call omitted");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckReplacementAndInsertion() {
    Runtime runtime;
    Check(pal4::GameDataAiPropertyTable::AddOrReplace(
              0x1000, 0x9000, runtime) == 1 &&
              At(runtime, "convert 16384") < At(runtime, "convert 16640") &&
              At(runtime, "convert 16640") < At(runtime, "find") &&
              runtime.calls.back() == "copy" &&
              runtime.copied[16] == 0x1234,
          "validated 68-byte record replaces indexed value");

    Runtime insertion;
    insertion.found = 0x5010;
    Check(pal4::GameDataAiPropertyTable::AddOrReplace(
              0x1000, 0x9000, insertion) == 1 &&
              At(insertion, "insert record") < At(insertion, "insert key"),
          "missing property key appends record then index");

    Runtime rebind;
    rebind.words[0x101c] = 0x203c;
    Check(pal4::GameDataAiPropertyTable::AddOrReplace(
              0x1000, 0x9000, rebind) == 1 &&
              rebind.calls[0] == "beginBindField, table:" &&
              rebind.calls[1] == "assign" &&
              rebind.calls[2] == "initialize property" &&
              rebind.calls[3] == "bind" &&
              rebind.calls[4] == "endBindField, table:",
          "mismatched count binds property structures before conversion");
}

void CheckValidationAndAppend() {
    for (int failure = 0; failure < 4; ++failure) {
        Runtime runtime;
        if (failure == 0) runtime.row_size = 1;
        if (failure == 1) runtime.first_index = 8;
        if (failure == 2) runtime.first_weight = 2.0f;
        if (failure == 3) runtime.second_weight = 2.0f;
        try {
            (void)pal4::GameDataAiPropertyTable::AddOrReplace(
                0x1000, 0x9000, runtime);
            Check(false, "invalid property row must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) ==
                      "ERROR <dbRecordBindTable< Record >::addRow >: property row" &&
                      runtime.calls.back() == "dump" &&
                      std::find(runtime.calls.begin(), runtime.calls.end(),
                                "find") == runtime.calls.end(),
                  "size, index and both weight sums reject before mutation");
        }
    }

    Runtime append;
    Check(pal4::GameDataAiPropertyTable::Append(0x1000, 0x9000, append) ==
              0x7777 && append.calls.back() == "append",
          "valid property record appends and forwards result");
    append = Runtime{};
    append.first_weight = 2.0f;
    try {
        (void)pal4::GameDataAiPropertyTable::Append(0x1000, 0x9000,
                                                     append);
        Check(false, "invalid appended record must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()).find("addRow") != std::string::npos &&
                  append.calls.back() == "dump",
              "append shares the real value-record validator");
    }
}

void CheckRefresh() {
    Runtime runtime;
    Check(pal4::GameDataAiPropertyTable::MarkAndRefresh(0x1000, runtime) ==
              0x1234 &&
              runtime.calls == std::vector<std::string>{"flag 1", "refresh"},
          "property refresh sets byte before dispatch and forwards result");
}

}  // namespace

int main() {
    try {
        CheckReplacementAndInsertion();
        CheckValidationAndAppend();
        CheckRefresh();
        std::cout << "game_data_ai_property_table_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
