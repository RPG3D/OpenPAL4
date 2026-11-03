#include "pal4/game_data_ai_text_record.h"

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

struct Runtime final : pal4::IGameDataAiTextRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
        return address + 0x10;
    }
};

std::size_t At(const Runtime& runtime, const std::string& value) {
    auto found = std::find(runtime.calls.begin(), runtime.calls.end(), value);
    Check(found != runtime.calls.end(), "expected call omitted");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckMixedReferences() {
    Runtime runtime;
    runtime.words[0x1028] = 0x2001;
    runtime.words[0x1018] = 0x3001;
    runtime.words[0x1004] = 0x4001;
    runtime.bytes[0x2000] = 0;
    runtime.bytes[0x3000] = 0xff;
    runtime.bytes[0x4000] = 2;
    Check(pal4::GameDataAiTextRecord::ResetPointers(0x1000, runtime) ==
              0x4000 &&
              runtime.bytes[0x4000] == 1 &&
              At(runtime, "free 8192") < At(runtime, "free 12288") &&
              At(runtime, "free 12288") < At(runtime, "byte 16384"),
          "fields release in reverse order and shared first text decrements");
    for (std::uint32_t offset : {4u, 8u, 12u, 24u, 28u, 32u,
                                 40u, 44u, 48u})
        Check(runtime.words[0x1000 + offset] == 0,
              "all three text pointer triplets are cleared");
}

void CheckReturnBranches() {
    Runtime final;
    final.words[0x1004] = 0x4001;
    final.bytes[0x4000] = 0;
    Check(pal4::GameDataAiTextRecord::ResetPointers(0x1000, final) ==
              0x4010 &&
              At(final, "free 16384") < At(final, "word 4100"),
          "last-field free result is forwarded before metadata clear");

    Runtime sentinel;
    sentinel.words[0x1004] = 0x4001;
    sentinel.bytes[0x4000] = 0xff;
    Check(pal4::GameDataAiTextRecord::ResetPointers(0x1000, sentinel) ==
              0x4010 &&
              At(sentinel, "free 16384") < sentinel.calls.size(),
          "0xff reference header also frees");

    Runtime empty;
    Check(pal4::GameDataAiTextRecord::ResetPointers(0x1000, empty) == 0 &&
              std::none_of(empty.calls.begin(), empty.calls.end(),
                           [](const std::string& call) {
                               return call.starts_with("free ");
                           }),
          "null references return zero without freeing");
}

void CheckDeletingWrapper() {
    Runtime runtime;
    runtime.words[0x1028] = 0x2001;
    runtime.bytes[0x2000] = 0;
    Check(pal4::GameDataAiTextRecord::Destroy(0x1000, 1, runtime) ==
              0x1000 &&
              At(runtime, "free 8192") < At(runtime, "free 4096") &&
              runtime.calls.back() == "free 4096",
          "deleting wrapper releases text before the record itself");
    Runtime retained;
    Check(pal4::GameDataAiTextRecord::Destroy(0x1000, 2, retained) ==
              0x1000 &&
              std::none_of(retained.calls.begin(), retained.calls.end(),
                           [](const std::string& call) {
                               return call == "free 4096";
                           }),
          "only low flag bit frees the record");
}

}  // namespace

int main() {
    try {
        CheckMixedReferences();
        CheckReturnBranches();
        CheckDeletingWrapper();
        std::cout << "game_data_ai_text_record_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
