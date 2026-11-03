#include "pal4/story_desc_data_table.h"

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct CreateRuntime final : pal4::IStoryDataTableRuntime {
    std::uint32_t allocation = 0x9000;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate:" + std::to_string(size));
        return allocation;
    }
    void ConstructBase(std::uint32_t object,
                       std::uint32_t name) override {
        calls.push_back("base:" + std::to_string(object) + ":" +
                        std::to_string(name));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        calls.push_back("byte:" + std::to_string(address));
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("word:" + std::to_string(address));
        words[address] = value;
    }
};

struct DestroyRuntime final : pal4::IStoryDataTableDestroyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
    }
    void ClearEffectData(std::uint32_t table) override {
        calls.push_back("clear:" + std::to_string(table));
    }
    void DestroyStoryText(std::uint32_t address) override {
        calls.push_back("text:" + std::to_string(address));
    }
    void ProcessElements(std::uint32_t address) override {
        calls.push_back("process:" + std::to_string(address));
        words[address - 8] = 0x9024;
    }
    void DestroyPropertyRecord(std::uint32_t address) override {
        calls.push_back("property:" + std::to_string(address));
    }
    void DestroyStoryRecord(std::uint32_t address) override {
        calls.push_back("record:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t DestroyBase(std::uint32_t table) override {
        calls.push_back("base:" + std::to_string(table));
        return 0x51;
    }
};

void CheckCreate() {
    CreateRuntime runtime;
    Check(pal4::StoryDescDataTable::Create(0x1234abcd, runtime) == 0x9000 &&
              runtime.calls.front() == "allocate:140" &&
              runtime.calls[1] == "base:36864:305441741" &&
              runtime.calls.size() == 15 &&
              runtime.words[0x9000] == 0x84476c,
          "StoryDesc uses its own vtable after base and three array seeds");
    for (const auto offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x9000 + offset] == 0xcd,
              "StoryDesc array seed retains low name byte");
        for (std::uint32_t word = 4; word <= 12; word += 4)
            Check(runtime.words[0x9000 + offset + word] == 0,
                  "StoryDesc pointer triplets are zeroed");
    }
    CreateRuntime failed;
    failed.allocation = 0;
    Check(pal4::StoryDescDataTable::Create(0x1234, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate:140"},
          "allocation failure skips base and field initialization");
}

void CheckDestroy() {
    DestroyRuntime runtime;
    constexpr std::uint32_t table = 0x7000;
    runtime.words[table + 96] = 0x9000;
    runtime.words[table + 100] = 0x9024;
    runtime.words[table + 112] = 0xa000;
    runtime.words[table + 116] = 0xa03c;
    runtime.words[table + 128] = 0xb000;
    Check(pal4::StoryDescDataTable::Destroy(table, runtime) == 0x51 &&
              runtime.calls.front() == "write:28672:8669036" &&
              runtime.calls[1] == "clear:28672" &&
              runtime.calls[2] == "text:36884" &&
              runtime.calls[3] == "text:36868" &&
              runtime.calls[5] == "process:28780" &&
              runtime.calls[10] == "property:40960" &&
              runtime.calls[15] == "record:36864" &&
              runtime.calls.back() == "base:28672",
          "StoryDesc destruction follows shared text, property, record and base order");
    for (const auto offset : {96u, 100u, 104u, 112u, 116u, 120u,
                              128u, 132u, 136u})
        Check(runtime.words[table + offset] == 0,
              "StoryDesc destruction clears all three storage triplets");
}

}  // namespace

int main() {
    try {
        CheckCreate();
        CheckDestroy();
        std::cout << "story_desc_data_table_lifecycle_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
