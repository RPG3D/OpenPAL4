#include "pal4/utilities_text_record_table.h"

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

struct Runtime final : pal4::IUtilitiesTextTableRuntime {
    std::uint32_t allocation = 0x1000;
    std::uint32_t clear_result = 0x1111;
    std::uint32_t effect_result = 0xdddd;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;

    std::uint32_t Allocate(std::uint32_t size) override {
        events.push_back("allocate:" + std::to_string(size));
        return allocation;
    }
    void ConstructEffect(std::uint32_t object,
                         std::uint32_t source) override {
        events.push_back("construct:" + std::to_string(object) + ":" +
                         std::to_string(source));
    }
    std::uint32_t DestructEffect(std::uint32_t object) override {
        events.push_back("destruct:" + std::to_string(object));
        return effect_result;
    }
    std::uint32_t ClearEffectData(std::uint32_t object) override {
        events.push_back("clear:" + std::to_string(object));
        return clear_result;
    }
    void ProcessElements(std::uint32_t object) override {
        events.push_back("process:" + std::to_string(object));
    }
    void ReleaseRecordText(std::uint32_t text) override {
        events.push_back("release:" + std::to_string(text));
    }
    std::uint32_t CleanupProperties5(std::uint32_t record) override {
        events.push_back("cleanup:" + std::to_string(record));
        return record + 0x55;
    }
    void DestroyBindingElement(std::uint32_t record,
                               std::uint32_t flag) override {
        events.push_back("binding:" + std::to_string(record) + ":" +
                         std::to_string(flag));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        events.push_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        events.push_back("byte:" + std::to_string(address));
    }
};

std::size_t Position(const Runtime& runtime, const std::string& event) {
    const auto found = std::find(runtime.events.begin(), runtime.events.end(),
                                 event);
    Check(found != runtime.events.end(), "expected native event occurred");
    return static_cast<std::size_t>(found - runtime.events.begin());
}

void CheckFactory() {
    Runtime runtime;
    Check(pal4::UtilitiesTextRecordTable::Create(0x1234, runtime) == 0x1000,
          "factory returns allocated object");
    Check(runtime.events.size() == 15 &&
              runtime.events[0] == "allocate:140" &&
              runtime.events[1] == "construct:4096:4660" &&
              runtime.events.back() == "word:4096",
          "factory allocates, constructs, initializes arrays and writes vtable last");
    Check(runtime.words[0x1000] == 0x8447ac &&
              runtime.bytes[0x105c] == 0x34 &&
              runtime.bytes[0x106c] == 0x34 &&
              runtime.bytes[0x107c] == 0x34,
          "factory installs vtable and source low byte in each array header");
    for (const auto offset : {96u, 100u, 104u, 112u, 116u, 120u,
                              128u, 132u, 136u}) {
        Check(runtime.words[0x1000 + offset] == 0,
              "all three pointer triplets start at zero");
    }

    Runtime failed;
    failed.allocation = 0;
    Check(pal4::UtilitiesTextRecordTable::Create(0x1234, failed) == 0 &&
              failed.events == std::vector<std::string>{"allocate:140"},
          "allocation failure returns null without constructing Effect");
}

void CheckDestroy() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2028;
    runtime.words[0x1070] = 0x3000;
    runtime.words[0x1074] = 0x3078;
    runtime.words[0x1080] = 0x4000;
    Check(pal4::UtilitiesTextRecordTable::Destroy(0x1000, runtime) == 0xdddd,
          "table destructor forwards Effect base destructor result");
    Check(runtime.words[0x1000] == 0x8447ac &&
              runtime.words[0x1060] == 0 && runtime.words[0x1064] == 0 &&
              runtime.words[0x1068] == 0 && runtime.words[0x1070] == 0 &&
              runtime.words[0x1074] == 0 && runtime.words[0x1078] == 0 &&
              runtime.words[0x1080] == 0 && runtime.words[0x1084] == 0 &&
              runtime.words[0x1088] == 0,
          "destructor clears all three array pointer triplets");
    Check(Position(runtime, "word:4096") < Position(runtime, "clear:4096") &&
              Position(runtime, "clear:4096") <
                  Position(runtime, "release:8196") &&
              Position(runtime, "release:8196") <
                  Position(runtime, "release:8216") &&
              Position(runtime, "release:8216") <
                  Position(runtime, "process:4204") &&
              Position(runtime, "process:4204") <
                  Position(runtime, "free:16384") &&
              Position(runtime, "free:16384") <
                  Position(runtime, "binding:12288:0") &&
              Position(runtime, "binding:12288:0") <
                  Position(runtime, "binding:12348:0") &&
              Position(runtime, "binding:12348:0") <
                  Position(runtime, "free:12288") &&
              Position(runtime, "free:12288") <
                  Position(runtime, "free:8192") &&
              Position(runtime, "free:8192") <
                  Position(runtime, "destruct:4096"),
          "record, binding and buffer cleanup preserve IDA order and strides");
    Check(std::none_of(runtime.events.begin(), runtime.events.end(),
                       [](const std::string& event) {
                           return event.starts_with("cleanup:");
                       }),
          "after end resets to begin the second record cleanup loop is empty");
}

void CheckEmptyDestroy() {
    Runtime runtime;
    runtime.words[0x1060] = 0;
    runtime.words[0x1064] = 0;
    runtime.words[0x1070] = 0;
    runtime.words[0x1074] = 0;
    runtime.words[0x1080] = 0;
    Check(pal4::UtilitiesTextRecordTable::Destroy(0x1000, runtime) ==
              0xdddd &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         "free:0") == 3 &&
              std::none_of(runtime.events.begin(), runtime.events.end(),
                           [](const std::string& event) {
                               return event.starts_with("release:") ||
                                      event.starts_with("binding:");
                           }),
          "empty table still releases three null array pointers");
}

void CheckArrayDestroy() {
    Runtime runtime;
    runtime.words[0x1004] = 0x2000;
    runtime.words[0x1008] = 0x203c;
    runtime.words[0x100c] = 0x2100;
    Check(pal4::UtilitiesTextRecordArray::Destroy(0x1000, runtime) == 0 &&
              runtime.events == std::vector<std::string>{
                  "cleanup:8192", "cleanup:8212", "cleanup:8232",
                  "free:8192", "word:4100", "word:4104", "word:4108"} &&
              runtime.words[0x1004] == 0 && runtime.words[0x1008] == 0 &&
              runtime.words[0x100c] == 0,
          "array destructor cleans 20-byte rows and resets its header");
}

void CheckClearRows() {
    Runtime runtime;
    runtime.words[0x1060] = 0x2000;
    runtime.words[0x1064] = 0x2028;
    Check(pal4::UtilitiesTextRecordTable::ClearRows(0x1000, runtime) ==
              0x2014 + 0x55 &&
              runtime.events == std::vector<std::string>{
                  "clear:4096", "cleanup:8192", "cleanup:8212",
                  "word:4196"} && runtime.words[0x1064] == 0x2000,
          "clear rows returns last cleanup result and retains allocation");
    runtime.events.clear();
    Check(pal4::UtilitiesTextRecordTable::ClearRows(0x1000, runtime) ==
              0x1111 &&
              runtime.events == std::vector<std::string>{
                  "clear:4096", "word:4196"},
          "empty clear forwards Effect clear result");
}

}  // namespace

int main() {
    try {
        CheckFactory();
        CheckDestroy();
        CheckEmptyDestroy();
        CheckArrayDestroy();
        CheckClearRows();
        std::cout << "utilities_text_record_table_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
