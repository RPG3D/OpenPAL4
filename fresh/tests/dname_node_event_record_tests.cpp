#include "pal4/dname_node_event_record.h"

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

constexpr std::uint32_t kRecord = 0x5000u;
constexpr std::uint32_t kText = 0x7000u;
constexpr std::uint32_t kBuffer = 0x9001u;

struct Runtime final : pal4::IDNameNodeEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::uint32_t length = 0u;
    bool fail_allocation = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        trace.push_back("word:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        trace.push_back("byte:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    std::uint32_t StringLength(std::uint32_t text) override {
        trace.push_back("length:" + std::to_string(text));
        return length;
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t value) override {
        trace.push_back("set-error:" + std::to_string(record) + ":" +
                        std::to_string(value));
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    void ReallocateString(std::uint32_t record,
                          std::uint32_t size) override {
        trace.push_back("reallocate:" + std::to_string(record) + ":" +
                        std::to_string(size));
        if (!fail_allocation) {
            words[record + 4u] = kBuffer;
            words[record + 12u] = size;
        }
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t size) override {
        trace.push_back("copy:" + std::to_string(destination) + ":" +
                        std::to_string(source) + ":" +
                        std::to_string(size));
    }
};

void EmptyAndNonempty() {
    Runtime empty;
    Check(pal4::DNameNodeEventRecord::SetString(
              kRecord, 0x11u, 0x22u, 0x8dc780u, empty) == kRecord &&
              empty.trace == std::vector<std::string>{
                  "word:20480:4294967295", "word:20484:0",
                  "byte:20488:128", "word:20492:0", "word:20496:0",
                  "word:20500:0", "word:20480:17", "word:20484:34",
                  "length:9291648", "set-error:20488:1"} &&
              empty.words[kRecord] == 0x11u &&
              empty.words[kRecord + 4u] == 0x22u &&
              empty.words[kRecord + 12u] == 0u,
          "empty text builds pair and zero-initialized string record");

    Runtime populated;
    populated.length = 4u;
    Check(pal4::DNameNodeEventRecord::SetString(
              kRecord, 0x33u, 0x44u, kText, populated) == kRecord &&
              populated.words[kRecord + 12u] == kBuffer &&
              populated.words[kRecord + 16u] == 4u &&
              populated.bytes[kBuffer + 4u] == 0u &&
              std::find(populated.trace.begin(), populated.trace.end(),
                        "set-error:20488:1") <
                  std::find(populated.trace.begin(), populated.trace.end(),
                            "reallocate:20488:4") &&
              std::find(populated.trace.begin(), populated.trace.end(),
                        "copy:36865:28672:4") <
                  std::find(populated.trace.begin(), populated.trace.end(),
                            "word:20496:4"),
          "nonempty text allocates then copies and null-terminates");
}

void ConstructNameNode() {
    Runtime runtime;
    runtime.words[kRecord] = 0x1234u;
    runtime.words[kRecord + 4u] = 0x5678u;
    Check(pal4::DNameNode::Construct(kRecord, runtime) == kRecord &&
          runtime.words.at(kRecord) == 0xffffffffu &&
          runtime.words.at(kRecord + 4u) == 0u &&
          runtime.trace == std::vector<std::string>{
              "word:20480:4294967295", "word:20484:0"},
          "DNameNode constructor resets both words in address order");
}

void FailurePaths() {
    Runtime overlong;
    overlong.length = 0xfffffffeu;
    bool length_caught = false;
    try {
        static_cast<void>(pal4::DNameNodeEventRecord::SetString(
            kRecord, 1u, 2u, kText, overlong));
    } catch (const std::length_error&) {
        length_caught = true;
    }
    Check(length_caught &&
              std::find_if(overlong.trace.begin(), overlong.trace.end(),
                           [](const std::string& item) {
                               return item.starts_with("set-error:");
                           }) == overlong.trace.end(),
          "native length limit rejects overflow before buffer work");

    Runtime failed;
    failed.length = 3u;
    failed.fail_allocation = true;
    bool allocation_caught = false;
    try {
        static_cast<void>(pal4::DNameNodeEventRecord::SetString(
            kRecord, 1u, 2u, kText, failed));
    } catch (const std::bad_alloc&) {
        allocation_caught = true;
    }
    Check(allocation_caught &&
              failed.trace.back() == "read:20492" &&
              failed.words[kRecord + 12u] == 0u,
          "failed text allocation has defined exception instead of null write");
}

}  // namespace

int main() {
    try {
        ConstructNameNode();
        EmptyAndNonempty();
        FailurePaths();
        std::cout << "dname_node_event_record_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
