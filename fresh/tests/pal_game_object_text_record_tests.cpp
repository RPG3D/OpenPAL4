#include "pal4/pal_game_object_text_record.h"

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

struct Runtime final : pal4::IPalGameObjectTextRecordRuntime {
    static constexpr std::uint32_t destination = 0x1000u;
    static constexpr std::uint32_t source = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    bool allocate = true;

    Runtime() {
        bytes[source] = 7u;
        words[source + 4u] = 0x8001u;
        words[source + 8u] = 3u;
        words[source + 12u] = 4u;
        words[source + 16u] = 0x4321u;
        bytes[0x8000u] = 1u;
        bytes[0x8001u] = 'a';
        bytes[0x8002u] = 'b';
        bytes[0x8003u] = 'c';
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t EmptyText() override { return 0x9001u; }
    void ThrowRange() override { throw std::out_of_range("range"); }
    void DetachAndReset(std::uint32_t record) override {
        calls.emplace_back("reset");
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        calls.emplace_back("allocate " + std::to_string(length) +
                           " " + std::to_string(mode));
        if (!allocate) return false;
        words[record + 4u] = 0xa001u;
        return true;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        words[record + 8u] = length;
    }
    void SetErrorFlag(std::uint32_t, std::uint8_t flag) override {
        calls.emplace_back("flag " + std::to_string(flag));
    }
    void CopyBytes(std::uint32_t target, std::uint32_t source_address,
                   std::uint32_t length) override {
        calls.emplace_back("copy " + std::to_string(length));
        for (std::uint32_t i = 0u; i < length; ++i)
            bytes[target + i] = bytes.at(source_address + i);
    }
    void ReleaseText(std::uint32_t record) override {
        calls.emplace_back("release " + std::to_string(record));
    }
    void FreeMemory(std::uint32_t address) override {
        calls.emplace_back("free " + std::to_string(address));
    }
};

void CheckCopy() {
    Runtime runtime;
    pal4::PalGameObjectTextRecord::InitializeAndCopy(
        0u, Runtime::source, runtime);
    Check(runtime.calls.empty(), "null destination has no effects");
    pal4::PalGameObjectTextRecord::InitializeAndCopy(
        Runtime::destination, Runtime::source, runtime);
    Check(runtime.words.at(Runtime::destination + 4u) == 0x8001u &&
          runtime.words.at(Runtime::destination + 8u) == 3u &&
          runtime.words.at(Runtime::destination + 16u) == 0x4321u &&
          runtime.bytes.at(0x8000u) == 2u &&
          runtime.calls == std::vector<std::string>{"flag 1"},
          "shareable text and tail metadata");

    Runtime unique;
    unique.bytes[0x8000u] = 0xfeu;
    pal4::PalGameObjectTextRecord::InitializeAndCopy(
        Runtime::destination, Runtime::source, unique);
    Check(unique.words.at(Runtime::destination + 4u) == 0xa001u &&
          unique.bytes.at(0xa003u) == 'c' &&
          unique.bytes.at(0xa004u) == 0u &&
          unique.calls == (std::vector<std::string>{
              "allocate 3 1", "copy 3"}),
          "saturated text gets owned copy");

    Runtime failed;
    failed.allocate = false;
    failed.bytes[0x8000u] = 0xffu;
    pal4::PalGameObjectTextRecord::InitializeAndCopy(
        Runtime::destination, Runtime::source, failed);
    Check(failed.words.at(Runtime::destination + 4u) == 0u &&
          failed.words.at(Runtime::destination + 16u) == 0x4321u,
          "failed allocation retains initialized record");
}

void CheckDelete() {
    Runtime runtime;
    Check(pal4::PalGameObjectTextRecord::Delete(
              Runtime::destination, 0u, runtime) ==
              Runtime::destination &&
          runtime.calls == std::vector<std::string>{"release 4096"},
          "nonowning delete only releases text");
    runtime.calls.clear();
    Check(pal4::PalGameObjectTextRecord::Delete(
              Runtime::destination, 3u, runtime) ==
              Runtime::destination &&
          runtime.calls == (std::vector<std::string>{
              "release 4096", "free 4096"}),
          "owning delete releases text before object");
}

}  // namespace

int main() {
    try {
        CheckCopy();
        CheckDelete();
        std::cout << "pal_game_object_text_record_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
