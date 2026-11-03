#include "pal4/indexed_equipment_data.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IndexedEquipmentDataRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t text{0xa001u};

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        events.push_back("word:" + std::to_string(address));
        words[address] = value;
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t value) override {
        Check(record == 0x978b00u && value == 1u,
              "fallback error flag arguments");
        events.emplace_back("error-flag");
    }
    void ReallocateText(std::uint32_t record,
                        std::uint32_t length) override {
        Check(record == 0x978b00u && length == 5u,
              "fallback text allocation arguments");
        events.emplace_back("allocate");
        words[record + 4u] = text;
    }
    void CopyErrorText(std::uint32_t destination) override {
        Check(destination == text, "ERROR text destination");
        events.emplace_back("copy-ERROR");
        for (std::uint32_t index = 0u; index < 5u; ++index)
            bytes[destination + index] =
                static_cast<std::uint8_t>("ERROR"[index]);
    }
    void RegisterCleanup() override {
        events.emplace_back("atexit");
    }
    void Free(std::uint32_t address) override {
        Check(address == text - 1u, "fallback allocation base");
        events.emplace_back("free");
    }
};

void ValidIndices() {
    Runtime runtime;
    for (const auto index : {0, 1, 12})
        Check(pal4::IndexedEquipmentData::Get(
                  0x9000u, index, runtime) ==
                  0x9098u + static_cast<std::uint32_t>(index) * 16u,
              "valid index returns direct 16-byte record");
    Check(runtime.events.empty(), "valid index has no global side effects");
}

void FirstAndRepeatedInvalidIndex() {
    Runtime runtime;
    runtime.bytes[0x978af8u] = 0xa2u;
    Check(pal4::IndexedEquipmentData::Get(
              0x9000u, -1, runtime) == 0x978b00u &&
              runtime.bytes[0x978af8u] == 0xa3u &&
              runtime.bytes[0x978b00u] == 0xffu &&
              runtime.words[0x978b04u] == runtime.text &&
              runtime.words[0x978b08u] == 5u &&
              runtime.words[0x978b0cu] == 0u &&
              runtime.bytes[runtime.text + 5u] == 0u,
          "first invalid index initializes global ERROR record");
    Check(runtime.events == std::vector<std::string>{
              "byte:9931512", "byte:9931520", "word:9931524",
              "word:9931528", "word:9931532", "error-flag",
              "allocate", "copy-ERROR", "word:9931528",
              "byte:40966", "atexit"},
          "fallback initialization follows IDA order");
    const auto count = runtime.events.size();
    Check(pal4::IndexedEquipmentData::Get(
              0x9000u, 13, runtime) == 0x978b00u &&
              runtime.events.size() == count &&
              runtime.bytes[0x978b00u] == 0xffu,
          "later invalid indices reuse the first fallback unchanged");
}

void CleanupBranches() {
    for (const auto count : {0u, 1u, 2u, 255u}) {
        Runtime runtime;
        runtime.words[0x978b04u] = runtime.text;
        runtime.words[0x978b08u] = 5u;
        runtime.words[0x978b0cu] = 7u;
        runtime.bytes[runtime.text - 1u] =
            static_cast<std::uint8_t>(count);
        pal4::IndexedEquipmentData::Cleanup(runtime);
        bool freed = false;
        for (const auto& event : runtime.events)
            if (event == "free") freed = true;
        Check(freed == (count == 0u || count == 255u),
              "zero and 255 refcounts free the allocation");
        if (!freed)
            Check(runtime.bytes[runtime.text - 1u] == count - 1u,
                  "ordinary shared reference is decremented");
        Check(runtime.words[0x978b04u] == 0u &&
                  runtime.words[0x978b08u] == 0u &&
                  runtime.words[0x978b0cu] == 0u,
              "cleanup clears all fallback text words");
    }
    Runtime empty;
    pal4::IndexedEquipmentData::Cleanup(empty);
    Check(empty.events.size() == 3u,
          "null text skips reference-count access and still clears words");
}

}  // namespace

int main() {
    ValidIndices();
    FirstAndRepeatedInvalidIndex();
    CleanupBranches();
    std::cout << "3 indexed-equipment data groups passed\n";
}
