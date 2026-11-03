#include "pal4/game_question_record.h"

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

struct Runtime final : pal4::IGameQuestionRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte:" + std::to_string(address));
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return address + 0x10;
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected call omitted");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckMixedReferences() {
    Runtime runtime;
    runtime.words[0x1038] = 0x2001;
    runtime.words[0x1028] = 0x3001;
    runtime.words[0x1018] = 0x4001;
    runtime.words[0x1008] = 0x5001;
    runtime.bytes[0x2000] = 0;
    runtime.bytes[0x3000] = 0xff;
    runtime.bytes[0x4000] = 3;
    runtime.bytes[0x5000] = 2;
    Check(pal4::GameQuestionRecord::Destroy(0x1000, runtime) == 0x5000 &&
              runtime.bytes[0x4000] == 2 &&
              runtime.bytes[0x5000] == 1 &&
              At(runtime, "free:8192") < At(runtime, "free:12288") &&
              At(runtime, "free:12288") < At(runtime, "byte:16384") &&
              At(runtime, "byte:16384") < At(runtime, "byte:20480"),
          "four text slots release in reverse order with reference counts");
    for (std::uint32_t offset : {8u, 12u, 16u, 24u, 28u, 32u,
                                 40u, 44u, 48u, 56u, 60u, 64u})
        Check(runtime.words[0x1000 + offset] == 0,
              "all text metadata words must clear");
}

void CheckReturnBranches() {
    Runtime final;
    final.words[0x1008] = 0x5001;
    final.bytes[0x5000] = 0;
    Check(pal4::GameQuestionRecord::Destroy(0x1000, final) == 0x5010 &&
              At(final, "free:20480") < At(final, "word:4104"),
          "last field forwards the native free result");

    Runtime empty;
    Check(pal4::GameQuestionRecord::Destroy(0x1000, empty) == 0 &&
              std::none_of(empty.calls.begin(), empty.calls.end(),
                           [](const std::string& call) {
                               return call.starts_with("free:");
                           }) &&
              empty.calls.size() == 12,
          "null fields clear all four triplets without freeing");
}

}  // namespace

int main() {
    try {
        CheckMixedReferences();
        CheckReturnBranches();
        std::cout << "game_question_record_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
