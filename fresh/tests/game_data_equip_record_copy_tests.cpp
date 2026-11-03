#include "pal4/game_data_equip_record_copy.h"

#include <algorithm>
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

struct Runtime final : pal4::IGameDataEquipRecordCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    Runtime() {
        for (std::uint32_t i = 0; i < 100; ++i)
            words[0x4000 + i * 4] = 1000 + i;
        for (std::uint32_t i = 0; i < 13; ++i) {
            bytes[0x4000 + 152 + i * 16] =
                static_cast<std::uint8_t>(i + 1);
            bytes[0x6000 + 152 + i * 16] = 0xaa;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void CopyConstructText(std::uint32_t destination,
                           std::uint32_t source) override {
        Check(destination - 0x6000 == source - 0x4000,
              "matching text offsets during construction");
        calls.push_back("construct " + std::to_string(destination));
    }
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        Check(destination - 0x6000 == source - 0x4000,
              "matching text offsets during assignment");
        calls.push_back("assign " + std::to_string(destination));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected operation missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckConstruct() {
    Runtime runtime;
    Check(pal4::GameDataEquipRecordCopy::Construct(
              0, 0x4000, runtime) == 0 && runtime.calls.empty(),
          "null target does not read or construct any field");
    Check(pal4::GameDataEquipRecordCopy::Construct(
              0x6000, 0x4000, runtime) == 0x4000 &&
              runtime.calls.size() == 74 &&
              runtime.calls.front() == "word 24576" &&
              runtime.calls.back() == "word 24972" &&
              At(runtime, "word 24724") <
                  At(runtime, "byte 24728") &&
              At(runtime, "byte 24728") <
                  At(runtime, "construct 24728") &&
              At(runtime, "construct 24920") <
                  At(runtime, "word 24936"),
          "construction copies 38 words, 13 text records and 10 tail words");
    for (std::uint32_t i = 0; i < 38; ++i)
        Check(runtime.words[0x6000 + i * 4] == 1000 + i,
              "leading DWORD data copied exactly");
    for (std::uint32_t i = 90; i < 100; ++i)
        Check(runtime.words[0x6000 + i * 4] == 1000 + i,
              "trailing DWORD data copied exactly");
    for (std::uint32_t i = 0; i < 13; ++i)
        Check(runtime.bytes[0x6000 + 152 + i * 16] == i + 1,
              "each text seed byte copied");
}

void CheckAssign() {
    Runtime runtime;
    Check(pal4::GameDataEquipRecordCopy::Assign(
              0x6000, 0x4000, runtime) == 0x6000 &&
              runtime.calls.size() == 61 &&
              runtime.calls.front() == "word 24576" &&
              At(runtime, "word 24724") <
                  At(runtime, "assign 24728") &&
              At(runtime, "assign 24920") <
                  At(runtime, "word 24936") &&
              runtime.calls.back() == "word 24972",
          "assignment preserves three-block order without constructing text");
    for (std::uint32_t i = 0; i < 13; ++i)
        Check(runtime.bytes[0x6000 + 152 + i * 16] == 0xaa,
              "assignment leaves text seed bytes to native text handler");
    for (std::uint32_t i : {0u, 35u, 36u, 37u, 90u, 99u})
        Check(runtime.words[0x6000 + i * 4] == 1000 + i,
              "assignment copies both raw DWORD spans");
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckAssign();
        std::cout << "game_data_equip_record_copy_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
