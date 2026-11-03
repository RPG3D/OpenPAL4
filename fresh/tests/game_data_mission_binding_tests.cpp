#include "pal4/game_data_mission_table.h"

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

struct Runtime final : pal4::IGameDataMissionBindingRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    void LogBind(std::string_view prefix,
                 std::uint32_t table_name) override {
        calls.push_back(std::string(prefix) + " " +
                        std::to_string(table_name));
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        calls.push_back("assign " + std::to_string(destination) + " " +
                        std::to_string(source));
    }
    void InitializeStructures(std::uint32_t binding) override {
        calls.push_back("initialize " + std::to_string(binding));
    }
    void BindEffects(std::uint32_t binding) override {
        calls.push_back("bind " + std::to_string(binding));
    }
};

void CheckMismatchedCounts() {
    Runtime runtime;
    runtime.words[0x1080] = 0x3000;
    runtime.words[0x1084] = 0x3004;
    runtime.words[0x1018] = 0x4000;
    runtime.words[0x101c] = 0x4078;
    pal4::GameDataMissionTable::RefreshBinding(0x1000, runtime);
    Check(runtime.calls == std::vector<std::string>{
                               "read 4224", "read 4228", "read 4120",
                               "read 4124", "beginBindField, table: 4148",
                               "assign 4204 4116", "initialize 4204",
                               "bind 4204", "endBindField, table: 4148"},
          "mismatched counts rebuild binding between exact log markers");
}

void CheckMatchingAndEmpty() {
    Runtime matching;
    matching.words[0x1080] = 0x3000;
    matching.words[0x1084] = 0x3008;
    matching.words[0x1018] = 0x4000;
    matching.words[0x101c] = 0x4078;
    pal4::GameDataMissionTable::RefreshBinding(0x1000, matching);
    Check(matching.calls == std::vector<std::string>{
                                "read 4224", "read 4228", "read 4120",
                                "read 4124"},
          "equal nonzero counts leave binding unchanged");

    Runtime empty;
    pal4::GameDataMissionTable::RefreshBinding(0x1000, empty);
    Check(empty.calls ==
              std::vector<std::string>{"read 4224", "read 4120"},
          "null arrays count as zero without reading their end pointers");

    Runtime bound_only;
    bound_only.words[0x1080] = 0x3000;
    bound_only.words[0x1084] = 0x3004;
    pal4::GameDataMissionTable::RefreshBinding(0x1000, bound_only);
    Check(bound_only.calls[2] == "read 4120" &&
              bound_only.calls.back() ==
                  "endBindField, table: 4148",
          "one-sided empty field array still triggers rebinding");
}

}  // namespace

int main() {
    try {
        CheckMismatchedCounts();
        CheckMatchingAndEmpty();
        std::cout << "game_data_mission_binding_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
