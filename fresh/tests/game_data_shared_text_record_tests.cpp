#include "pal4/game_data_shared_text_record.h"

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

struct Runtime final : pal4::IGameDataSharedTextRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t free_result = 0xface;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read word " + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read byte " + std::to_string(address));
        return bytes[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write word " + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write byte " + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
        return free_result;
    }
};

void CheckRelease(std::uint32_t data, std::uint8_t references,
                  std::uint32_t expected_result, bool free_expected,
                  std::uint32_t offset = 12) {
    Runtime runtime;
    runtime.words[0x1000 + offset] = data;
    runtime.words[0x1004 + offset] = 12;
    runtime.words[0x1008 + offset] = 20;
    if (data) runtime.bytes[data - 1] = references;
    const auto result = offset == 12
        ? pal4::GameDataSharedTextRecord::ReleaseFirst(0x1000, runtime)
        : pal4::GameDataSharedTextRecord::ReleaseSecond(0x1000, runtime);
    Check(result == expected_result && runtime.words[0x1000 + offset] == 0 &&
              runtime.words[0x1004 + offset] == 0 &&
              runtime.words[0x1008 + offset] == 0 &&
              runtime.words[0x1000] == 0x842bd4,
          "release result, three empty fields and base vtable");
    const std::string free_call = "free " + std::to_string(data - 1);
    bool freed = false;
    for (const auto& call : runtime.calls) freed |= call == free_call;
    Check(freed == free_expected, "free decision follows reference byte");
    if (data && !free_expected)
        Check(runtime.bytes[data - 1] == references - 1,
              "shared reference decremented");
    Check(runtime.calls.back() == "write word 4096",
          "base vtable is the last write");
}

}  // namespace

int main() {
    try {
        CheckRelease(0, 0, 0, false);
        CheckRelease(0x2001, 3, 0x2000, false);
        CheckRelease(0x2001, 1, 0x2000, false);
        CheckRelease(0x2001, 0, 0xface, true);
        CheckRelease(0x2001, 0xff, 0xface, true);
        CheckRelease(0, 0, 0, false, 20);
        CheckRelease(0x3001, 2, 0x3000, false, 20);
        CheckRelease(0x3001, 0xff, 0xface, true, 20);
        std::cout << "game_data_shared_text_record_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
