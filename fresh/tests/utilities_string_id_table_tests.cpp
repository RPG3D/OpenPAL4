#include "pal4/utilities_string_id_table.h"

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

struct FactoryRuntime final : pal4::IUtilitiesStringIdFactoryRuntime {
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
    Check(pal4::UtilitiesStringIdTable::Create(0x1234, runtime) ==
              0x1000 &&
              runtime.calls.size() == 15 &&
              runtime.calls.front() == "allocate 140" &&
              runtime.calls[1] == "construct 4096 4660" &&
              runtime.calls.back() == "word 4096" &&
              runtime.words[0x1000] == 0x84484c,
          "factory constructs StringId Effect and installs vtable last");
    for (const auto offset : {92u, 108u, 124u})
        Check(runtime.bytes[0x1000 + offset] == 0x34 &&
                  runtime.words[0x1000 + offset + 4] == 0 &&
                  runtime.words[0x1000 + offset + 8] == 0 &&
                  runtime.words[0x1000 + offset + 12] == 0,
              "three headers retain source byte and null pointers");

    FactoryRuntime failed;
    failed.allocation = 0;
    Check(pal4::UtilitiesStringIdTable::Create(0x1234, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate 140"},
          "allocation failure does not construct Effect");
}

struct LifetimeRuntime final : pal4::IUtilitiesStringIdLifetimeRuntime {
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
    std::uint32_t DestructEffect(std::uint32_t object) override {
        calls.push_back("destruct " + std::to_string(object));
        return 0x77;
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
    runtime.words[0x1064] = 0x2010;
    runtime.words[0x1080] = 0x3000;
    runtime.words[0x1070] = 0x4000;
    runtime.words[0x1074] = 0x4078;
    Check(pal4::UtilitiesStringIdTable::Destroy(0x1000, runtime) ==
              0x77 &&
              runtime.words[0x1000] == 0x84484c,
          "destructor reinstalls StringId vtable and forwards Effect result");
    Check(Position(runtime, "word 4096") <
              Position(runtime, "clear 4096") &&
              Position(runtime, "clear 4096") <
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
          "POD rows need no per-record cleanup before buffers release");
    for (const auto offset : {96u, 100u, 104u, 112u, 116u,
                              120u, 128u, 132u, 136u})
        Check(runtime.words[0x1000 + offset] == 0,
              "all array pointer triplets reset");

    LifetimeRuntime empty;
    empty.words[0x1060] = 0;
    empty.words[0x1080] = 0;
    empty.words[0x1070] = 0;
    empty.words[0x1074] = 0;
    Check(pal4::UtilitiesStringIdTable::Destroy(0x1000, empty) ==
              0x77 &&
              std::count(empty.calls.begin(), empty.calls.end(),
                         "free 0") == 3 &&
              std::none_of(empty.calls.begin(), empty.calls.end(),
                           [](const std::string& call) {
                               return call.starts_with("binding ");
                           }),
          "empty destructor releases only array buffers and Effect");
}

struct ClearRuntime final : pal4::IUtilitiesStringIdClearRuntime {
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
};

void CheckClearRows() {
    ClearRuntime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2010;
    Check(pal4::UtilitiesStringIdTable::ClearRows(0x1000, runtime) ==
              0x2010 &&
              runtime.calls ==
                  std::vector<std::string>{"clear 4096", "word 4196"} &&
              runtime.words[0x1064] == 0x2000,
          "clear returns old end while retaining row capacity");
    runtime.calls.clear();
    Check(pal4::UtilitiesStringIdTable::ClearRows(0x1000, runtime) ==
              0x2000 &&
              runtime.calls ==
                  std::vector<std::string>{"clear 4096", "word 4196"},
          "empty clear returns the prior begin/end pointer");
}

}  // namespace

int main() {
    try {
        CheckFactory();
        CheckDestroy();
        CheckClearRows();
        std::cout << "utilities_string_id_table_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
