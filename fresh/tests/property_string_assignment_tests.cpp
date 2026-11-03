#include "pal4/property_string_assignment.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyStringRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint8_t allocate_result{1u};
    std::uint8_t detach_result{0x77u};
    std::uint32_t allocated_data{0x9000u};

    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        calls.push_back("copy:" + std::to_string(destination) + ":" +
                        std::to_string(source) + ":" +
                        std::to_string(length));
        for (std::uint32_t i = 0; i < length; ++i)
            bytes[destination + i] = bytes.at(source + i);
    }
    std::uint8_t AllocateText(std::uint32_t record,
                              std::uint32_t length,
                              std::uint8_t mode) override {
        calls.push_back("allocate:" + std::to_string(record) + ":" +
                        std::to_string(length) + ":" +
                        std::to_string(mode));
        if (allocate_result != 0u) {
            words[record + 4u] = allocated_data;
            words[record + 12u] = length;
        }
        return allocate_result;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        calls.push_back("size:" + std::to_string(record) + ":" +
                        std::to_string(length));
        words[record + 8u] = length;
        bytes[words.at(record + 4u) + length] = 0u;
    }
    std::uint8_t DetachAndReset(std::uint32_t record) override {
        calls.push_back("detach:" + std::to_string(record));
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
        return detach_result;
    }
    void ThrowRange() override { throw std::out_of_range("property string"); }
};

void PutRecord(Runtime& runtime, std::uint32_t record,
               std::uint32_t data, std::uint32_t length,
               std::uint32_t capacity) {
    runtime.words[record + 4u] = data;
    runtime.words[record + 8u] = length;
    runtime.words[record + 12u] = capacity;
}

void TestSharedCopy() {
    Runtime runtime;
    const auto destination = 0x1020u;
    PutRecord(runtime, destination, 0x3000u, 5u, 10u);
    PutRecord(runtime, 0x2000u, 0x4003u, 3u, 8u);
    runtime.bytes[0x2fffu] = 0u;
    runtime.bytes[0x4002u] = 2u;
    Check(pal4::PropertyStringAssignment::Assign(
              0x20u, 0x1000u, 0x2000u, runtime) == 3u &&
              runtime.words.at(destination + 4u) == 0x4003u &&
              runtime.words.at(destination + 8u) == 3u &&
              runtime.words.at(destination + 12u) == 8u &&
              runtime.bytes.at(0x4002u) == 3u &&
              runtime.calls.front() == "free:12287" &&
              runtime.calls.back() == "byte:16386:3",
          "shared source replaces destination, updates header, returns data low byte");

    PutRecord(runtime, destination, 0x3100u, 1u, 4u);
    runtime.bytes[0x30ffu] = 2u;
    runtime.bytes[0x4002u] = 1u;
    runtime.calls.clear();
    (void)pal4::PropertyStringAssignment::Assign(
        0x20u, 0x1000u, 0x2000u, runtime);
    Check(runtime.bytes.at(0x30ffu) == 1u &&
              runtime.calls.front() == "byte:12543:1",
          "shared destination header is decremented");
}

void TestFallbackAndDeepCopy() {
    Runtime runtime;
    PutRecord(runtime, 0x1020u, 0u, 0u, 0u);
    PutRecord(runtime, 0x2000u, 0u, 1u, 7u);
    runtime.bytes[pal4::PropertyStringAssignment::kEmptyLogMessage - 1u] = 0u;
    (void)pal4::PropertyStringAssignment::Assign(
        0x20u, 0x1000u, 0x2000u, runtime);
    Check(runtime.words.at(0x1024u) ==
              pal4::PropertyStringAssignment::kEmptyLogMessage &&
              runtime.bytes.at(
                  pal4::PropertyStringAssignment::kEmptyLogMessage - 1u) == 1u,
          "null source data uses the shared empty-log address");

    Runtime deep;
    PutRecord(deep, 0x1020u, 0u, 0u, 0u);
    PutRecord(deep, 0x2000u, 0x4000u, 3u, 5u);
    deep.bytes[0x3fffu] = 0xfeu;
    deep.bytes[0x4000u] = 'a';
    deep.bytes[0x4001u] = 'b';
    deep.bytes[0x4002u] = 'c';
    Check(pal4::PropertyStringAssignment::Assign(
              0x20u, 0x1000u, 0x2000u, deep) == 3u &&
              deep.calls.front() == "allocate:4128:3:1" &&
              deep.words.at(0x1024u) == 0x9000u &&
              deep.words.at(0x1028u) == 3u &&
              deep.bytes.at(0x9000u) == 'a' &&
              deep.bytes.at(0x9002u) == 'c' &&
              deep.bytes.at(0x9003u) == 0u,
          "saturated source header takes deep copy and terminates text");

    deep.allocate_result = 0u;
    deep.calls.clear();
    Check(pal4::PropertyStringAssignment::Assign(
              0x20u, 0x1000u, 0x2000u, deep) == 0u &&
              deep.calls.size() == 1u &&
              deep.calls.front() == "allocate:4128:3:1",
          "failed allocation avoids copy and size writes");

    PutRecord(deep, 0x2000u, 0x4000u, 0u, 5u);
    deep.allocate_result = 1u;
    deep.calls.clear();
    Check(pal4::PropertyStringAssignment::Assign(
              0x20u, 0x1000u, 0x2000u, deep) == 0u &&
              deep.calls.front() == "allocate:4128:0:1" &&
              deep.bytes.at(0x9000u) == 0u,
          "empty source still takes allocation path");
}

void TestSelfAssignment() {
    Runtime runtime;
    PutRecord(runtime, 0x1020u, 0x3000u, 1u, 5u);
    Check(pal4::PropertyStringAssignment::Assign(
              0x20u, 0x1000u, 0x1020u, runtime) == 0x77u &&
              runtime.calls == std::vector<std::string>{
                  "detach:4128", "copy:1:0:0",
                  "allocate:4128:1:0", "size:4128:1",
                  "detach:4128"},
          "self assignment follows both detach calls and zero-byte copy");
    Check(pal4::PropertyStringAssignment::ElementAddress(
              0x20u, 0x1000u) == 0x1020u,
          "element-address getter adds stored offset");
}

}  // namespace

int main() {
    TestSharedCopy();
    TestFallbackAndDeepCopy();
    TestSelfAssignment();
}
