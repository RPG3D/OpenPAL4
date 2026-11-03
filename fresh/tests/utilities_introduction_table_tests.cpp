#include "pal4/utilities_introduction_table.h"

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

struct FactoryRuntime final : pal4::IUtilitiesIntroductionFactoryRuntime {
    std::uint32_t allocation = 0x1000;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate " + std::to_string(size));
        return allocation;
    }
    void ConstructEffect(std::uint32_t object,
                         std::uint32_t source) override {
        calls.push_back("construct " + std::to_string(object) + " " +
                        std::to_string(source));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
};

void CheckFactory() {
    FactoryRuntime runtime;
    Check(pal4::UtilitiesIntroductionTable::Create(
              0x1234, runtime) == 0x1000 &&
              runtime.calls.size() == 15 &&
              runtime.calls.front() == "allocate 140" &&
              runtime.calls[1] == "construct 4096 4660" &&
              runtime.calls.back() == "word 4096" &&
              runtime.words[0x1000] == 0x8447ec,
          "factory constructs 140-byte Effect and installs vtable last");
    for (const auto offset : {92u, 108u, 124u})
        Check(runtime.bytes[0x1000 + offset] == 0x34 &&
                  runtime.words[0x1000 + offset + 4] == 0 &&
                  runtime.words[0x1000 + offset + 8] == 0 &&
                  runtime.words[0x1000 + offset + 12] == 0,
              "all three headers retain source byte and null pointers");

    FactoryRuntime failed;
    failed.allocation = 0;
    Check(pal4::UtilitiesIntroductionTable::Create(
              0x1234, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate 140"},
          "failed allocation does not construct Effect");
}

struct LifetimeRuntime final : pal4::IUtilitiesIntroductionLifetimeRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void ClearEffect(std::uint32_t object) override {
        calls.push_back("clear " + std::to_string(object));
    }
    std::uint32_t DestructEffect(std::uint32_t object) override {
        calls.push_back("destruct " + std::to_string(object));
        return 0x77;
    }
    void ReleaseText(std::uint32_t address) override {
        calls.push_back("release " + std::to_string(address));
    }
    void ProcessElements(std::uint32_t address) override {
        calls.push_back("process " + std::to_string(address));
    }
    void DestroyBinding(std::uint32_t address,
                        std::uint32_t flag) override {
        calls.push_back("binding " + std::to_string(address) + " " +
                        std::to_string(flag));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
};

std::size_t Position(const LifetimeRuntime& runtime,
                     const std::string& call) {
    const auto found = std::find(runtime.calls.begin(),
                                 runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "expected native call occurred");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckDestroy() {
    LifetimeRuntime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2048;
    runtime.words[0x1080] = 0x3000;
    runtime.words[0x1070] = 0x4000;
    runtime.words[0x1074] = 0x4078;
    Check(pal4::UtilitiesIntroductionTable::Destroy(
              0x1000, runtime) == 0x77 &&
              runtime.words[0x1000] == 0x8447ec,
          "destructor installs Introduction vtable and forwards Effect result");
    Check(Position(runtime, "word 4096") <
              Position(runtime, "clear 4096") &&
              Position(runtime, "clear 4096") <
                  Position(runtime, "release 8212") &&
              Position(runtime, "release 8212") <
                  Position(runtime, "release 8196") &&
              Position(runtime, "release 8248") <
                  Position(runtime, "word 4196") &&
              Position(runtime, "word 4196") <
                  Position(runtime, "process 4204") &&
              Position(runtime, "process 4204") <
                  Position(runtime, "free 12288") &&
              Position(runtime, "free 12288") <
                  Position(runtime, "binding 16384 0") &&
              Position(runtime, "binding 16444 0") <
                  Position(runtime, "free 16384") &&
              Position(runtime, "free 16384") <
                  Position(runtime, "free 8192") &&
              Position(runtime, "free 8192") <
                  Position(runtime, "destruct 4096"),
          "two text fields per 36-byte row and buffers release in IDA order");
    for (const auto offset : {96u, 100u, 104u, 112u, 116u,
                              120u, 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "all array pointer triplets reset");

    LifetimeRuntime empty;
    empty.words[0x1060] = 0;
    empty.words[0x1064] = 0;
    empty.words[0x1080] = 0;
    empty.words[0x1070] = 0;
    empty.words[0x1074] = 0;
    Check(pal4::UtilitiesIntroductionTable::Destroy(
              0x1000, empty) == 0x77 &&
              std::count(empty.calls.begin(), empty.calls.end(),
                         "free 0") == 3 &&
              std::none_of(empty.calls.begin(), empty.calls.end(),
                           [](const std::string& call) {
                               return call.starts_with("release ") ||
                                      call.starts_with("binding ");
                           }),
          "empty destructor skips record and binding cleanup");
}

struct ArrayRuntime final : pal4::IUtilitiesIntroductionArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void DestroyRecord(std::uint32_t record) override {
        calls.push_back("destroy " + std::to_string(record));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
};

void CheckArrayDestroy() {
    ArrayRuntime runtime;
    runtime.words[0x1004] = 0x2000;
    runtime.words[0x1008] = 0x206c;
    Check(pal4::UtilitiesIntroductionArray::Destroy(
              0x1000, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                  "destroy 8192", "destroy 8228", "destroy 8264",
                  "free 8192", "word 4100", "word 4104", "word 4108"} &&
              runtime.words[0x1004] == 0 &&
              runtime.words[0x1008] == 0 &&
              runtime.words[0x100c] == 0,
          "array destructor clears three 36-byte records and header");
}

struct ClearRuntime final : pal4::IUtilitiesIntroductionClearRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    std::uint32_t ClearEffect(std::uint32_t object) override {
        calls.push_back("clear " + std::to_string(object));
        return 0x11;
    }
    std::uint32_t DestroyRecord(std::uint32_t record) override {
        calls.push_back("destroy " + std::to_string(record));
        return record + 4;
    }
};

void CheckClearRows() {
    ClearRuntime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2048;
    Check(pal4::UtilitiesIntroductionTable::ClearRows(
              0x1000, runtime) == 0x2028 &&
              runtime.calls == std::vector<std::string>{
                  "clear 4096", "destroy 8192", "destroy 8228",
                  "word 4196"} &&
              runtime.words[0x1064] == 0x2000,
          "clear returns last row cleanup and retains allocated buffer");
    runtime.calls.clear();
    Check(pal4::UtilitiesIntroductionTable::ClearRows(
              0x1000, runtime) == 0x11 &&
              runtime.calls == std::vector<std::string>{
                  "clear 4096", "word 4196"},
          "empty clear forwards Effect clear result");
}

}  // namespace

int main() {
    try {
        CheckFactory();
        CheckDestroy();
        CheckArrayDestroy();
        CheckClearRows();
        std::cout << "utilities_introduction_table_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
