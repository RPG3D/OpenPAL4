#include "pal4/combat_system_config_init.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSystemConfigInitRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t length = 0u;
    bool allocate_first = true;
    std::uint32_t next_data = 0x9000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint32_t StringLength(std::uint32_t address) override {
        Check(address == pal4::CombatSystemConfigInit::kErrorString,
              "shared error string address");
        calls.emplace_back("length");
        return length;
    }
    bool AllocateText(std::uint32_t record, std::uint32_t size,
                      std::uint8_t mode) override {
        calls.emplace_back("allocate:" + std::to_string(record) + ":" +
                           std::to_string(size) + ":" +
                           std::to_string(mode));
        if (!allocate_first) return false;
        words[record + 4u] = next_data;
        next_data += 0x100u;
        return true;
    }
    void ReallocateText(std::uint32_t record,
                        std::uint32_t size) override {
        calls.emplace_back("reallocate:" + std::to_string(record) + ":" +
                           std::to_string(size));
        words[record + 4u] = next_data;
        next_data += 0x100u;
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t size) override {
        calls.emplace_back("copy:" + std::to_string(destination) + ":" +
                           std::to_string(source) + ":" +
                           std::to_string(size));
    }
    std::uint8_t SetErrorFlag(std::uint32_t record,
                              std::uint8_t flag) override {
        calls.emplace_back("flag:" + std::to_string(record) + ":" +
                           std::to_string(flag));
        return flag;
    }
    [[noreturn]] void ThrowLength() override {
        throw std::length_error("text length");
    }
};

void FreshInitialization() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x2000u;
    Check(pal4::CombatSystemConfigInit::Initialize(
              object, 0x5au, runtime) == object,
          "initializer returns object");
    Check(runtime.bytes[object + 20u] == 0x5au &&
              runtime.bytes[object + 36u] == 0x5au &&
              runtime.bytes[object + 52u] == 0x5au &&
              runtime.words[object + 16u] == 1u &&
              runtime.words[object + 28u] == 0u &&
              runtime.bytes[0x9000u] == 0u &&
              runtime.bytes[object + 68u] == 1u &&
              runtime.bytes[object + 69u] == 0u &&
              runtime.bytes[object + 70u] == 0u,
          "seed, empty text and tail flags");
    Check(runtime.calls[0] == "byte:8212:90" &&
              runtime.calls[9] == "byte:8244:90" &&
              runtime.calls[12] == "word:8204:0" &&
              runtime.calls[17] == "length" &&
              runtime.calls[18] == "allocate:8212:0:1" &&
              runtime.calls[19] == "copy:36864:9291648:0",
          "constructor and string initialization order");
    Runtime skipped;
    skipped.allocate_first = false;
    Check(pal4::CombatSystemConfigInit::InitializeStrings(
              object, skipped) == 1u &&
              skipped.words[object + 24u] == 0u,
          "failed zero-length first allocation still sets later flags");
}

void ExistingAndNonemptyRecords() {
    Runtime shared;
    constexpr std::uint32_t object = 0x3000u;
    shared.words[object + 40u] = 0x7001u;
    shared.words[object + 56u] = 0x8001u;
    shared.bytes[0x7000u] = 3u;
    shared.bytes[0x8000u] = 2u;
    Check(pal4::CombatSystemConfigInit::InitializeStrings(
              object, shared) == 0u &&
              shared.bytes[0x7000u] == 2u &&
              shared.bytes[0x8000u] == 1u,
          "empty assignment decrements shared references");

    Runtime nonempty;
    nonempty.length = 5u;
    nonempty.words[object + 40u] = 0x7001u;
    nonempty.bytes[0x7000u] = 2u;
    nonempty.words[object + 56u] = 0x8001u;
    nonempty.bytes[0x8000u] = 0xffu;
    nonempty.words[object + 64u] = 6u;
    Check(pal4::CombatSystemConfigInit::InitializeStrings(
              object, nonempty) == 5u &&
              nonempty.words[object + 28u] == 5u &&
              nonempty.words[object + 44u] == 5u &&
              nonempty.words[object + 60u] == 5u &&
              nonempty.words[object + 40u] != 0x7001u &&
              nonempty.words[object + 56u] == 0x8001u &&
              nonempty.bytes[0x8006u] == 0u,
          "nonempty source reallocates shared text and reuses capacity");
}

}  // namespace

int main() {
    try {
        FreshInitialization();
        ExistingAndNonemptyRecords();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
