#include "pal4/game_data_mission_record_copy.h"

#include <algorithm>
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

struct Runtime final : pal4::IGameDataMissionRecordCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint32_t AssignText(std::uint32_t destination,
                             std::uint32_t source) override {
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(words[destination + step] == 0,
                  "text pointer triplet must be zero before assignment");
        calls.push_back("assign " + std::to_string(destination) + " " +
                        std::to_string(source));
        return destination + 12;
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto it = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(it != runtime.calls.end(), "expected record-copy operation missing");
    return static_cast<std::size_t>(it - runtime.calls.begin());
}

void CheckConstruction() {
    Runtime runtime;
    for (std::uint32_t i = 0; i < 5; ++i)
        runtime.words[0x2000 + i * 4] = 100 + i;
    for (std::uint32_t offset : {20u, 36u, 52u}) {
        runtime.bytes[0x2000 + offset] =
            static_cast<std::uint8_t>(offset);
        for (std::uint32_t step : {4u, 8u, 12u})
            runtime.words[0x3000 + offset + step] = 0xdead;
    }
    Check(pal4::GameDataMissionRecordCopy::Construct(
              0x3000, 0x2000, runtime) == 0x3040,
          "construction returns final text assignment result");
    for (std::uint32_t i = 0; i < 5; ++i)
        Check(runtime.words[0x3000 + i * 4] == 100 + i,
              "first five scalar words are copied");
    for (std::uint32_t offset : {20u, 36u, 52u}) {
        Check(runtime.bytes[0x3000 + offset] == offset,
              "text seed byte is copied");
        for (std::uint32_t step : {4u, 8u, 12u})
            Check(runtime.words[0x3000 + offset + step] == 0,
                  "text pointer triplet is cleared");
    }
    Check(At(runtime, "word 12288 100") <
              At(runtime, "byte 12308 20") &&
              At(runtime, "word 12320 0") <
                  At(runtime, "assign 12308 8212") &&
              At(runtime, "assign 12308 8212") <
                  At(runtime, "assign 12324 8228") &&
              At(runtime, "assign 12324 8228") <
                  At(runtime, "assign 12340 8244"),
          "scalar copy precedes three ordered text assignments");
}

void CheckNullDestination() {
    Runtime runtime;
    Check(pal4::GameDataMissionRecordCopy::Construct(
              0, 0x2000, runtime) == 0 && runtime.calls.empty(),
          "null destination skips all reads, writes and text assignment");
}

void CheckAssignment() {
    Runtime runtime;
    for (std::uint32_t i = 0; i < 5; ++i)
        runtime.words[0x2000 + i * 4] = 200 + i;
    Check(pal4::GameDataMissionRecordCopy::Assign(
              0x3000, 0x2000, runtime) == 0x3000,
          "assignment returns destination record");
    for (std::uint32_t i = 0; i < 5; ++i)
        Check(runtime.words[0x3000 + i * 4] == 200 + i,
              "assignment copies five scalar words");
    Check(runtime.calls == std::vector<std::string>{
                               "word 12288 200", "word 12292 201",
                               "word 12296 202", "word 12300 203",
                               "word 12304 204", "assign 12308 8212",
                               "assign 12324 8228", "assign 12340 8244"},
          "assignment processes scalar words before ordered text members");

    Runtime self;
    Check(pal4::GameDataMissionRecordCopy::Assign(
              0x2000, 0x2000, self) == 0x2000 &&
              At(self, "assign 8212 8212") <
                  At(self, "assign 8228 8228") &&
              At(self, "assign 8228 8228") <
                  At(self, "assign 8244 8244"),
          "self-assignment still forwards each text member to string routine");
}

}  // namespace

int main() {
    try {
        CheckConstruction();
        CheckNullDestination();
        CheckAssignment();
        std::cout << "game_data_mission_record_copy_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
