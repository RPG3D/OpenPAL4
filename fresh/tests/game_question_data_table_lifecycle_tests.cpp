#include "pal4/game_question_data_table.h"

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

struct DestroyRuntime final : pal4::IGameQuestionDataTableDestroyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool change_end = true;

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
    void DestroyQuestionRecord(std::uint32_t record) override {
        calls.push_back("record:" + std::to_string(record));
    }
    void ProcessElements(std::uint32_t elements) override {
        calls.push_back("process:" + std::to_string(elements));
        if (change_end) words[elements - 8] = 0x9000 + 76;
    }
    void DestroyPropertyRecord(std::uint32_t record) override {
        calls.push_back("property:" + std::to_string(record));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t DestroyBase(std::uint32_t table) override {
        calls.push_back("base:" + std::to_string(table));
        return 0x6a6a;
    }
};

void CheckCreate() {
    CreateRuntime runtime;
    Check(pal4::GameQuestionDataTable::Create(0x123456e7, runtime) ==
              0x9000 &&
              runtime.calls.front() == "allocate:140" &&
              runtime.calls.size() == 15 &&
              runtime.words[0x9000] == 0x84478c,
          "question table has 140-byte allocation and its own vtable");
    for (const auto offset : {92u, 108u, 124u}) {
        Check(runtime.bytes[0x9000 + offset] == 0xe7,
              "array seed keeps low name byte");
        for (std::uint32_t word = 4; word <= 12; word += 4)
            Check(runtime.words[0x9000 + offset + word] == 0,
                  "three pointer triplets are zeroed");
    }
    CreateRuntime failed;
    failed.allocation = 0;
    Check(pal4::GameQuestionDataTable::Create(0x1234, failed) == 0 &&
              failed.calls == std::vector<std::string>{"allocate:140"},
          "allocation failure skips base construction");
}

void CheckDestroy() {
    constexpr std::uint32_t table = 0x7000;
    DestroyRuntime runtime;
    runtime.words[table + 96] = 0x9000;
    runtime.words[table + 100] = 0x9000 + 2 * 76;
    runtime.words[table + 112] = 0xa000;
    runtime.words[table + 116] = 0xa000 + 2 * 60;
    runtime.words[table + 128] = 0xb000;
    Check(pal4::GameQuestionDataTable::Destroy(table, runtime) == 0x6a6a &&
              runtime.calls == std::vector<std::string>{
                  "write:28672:8669068", "clear:28672",
                  "record:36864", "record:36940",
                  "write:28772:36864", "process:28780", "free:45056",
                  "write:28800:0", "write:28804:0", "write:28808:0",
                  "property:40960", "property:41020", "free:40960",
                  "write:28784:0", "write:28788:0", "write:28792:0",
                  "record:36864", "free:36864",
                  "write:28768:0", "write:28772:0", "write:28776:0",
                  "base:28672"},
          "two 76-byte passes bracket property cleanup and reread end");
    for (const auto offset : {96u, 100u, 104u, 112u, 116u, 120u,
                              128u, 132u, 136u})
        Check(runtime.words[table + offset] == 0,
              "question storage triplets are zero after destruction");

    DestroyRuntime empty;
    empty.change_end = false;
    Check(pal4::GameQuestionDataTable::Destroy(table, empty) == 0x6a6a &&
              empty.calls.back() == "base:28672",
          "empty table still releases all storage and the Effect base");
    for (const auto& call : empty.calls)
        Check(call.rfind("record:", 0) != 0 &&
                  call.rfind("property:", 0) != 0,
              "empty row and property ranges skip record destructors");
}

}  // namespace

int main() {
    try {
        CheckCreate();
        CheckDestroy();
        std::cout << "game_question_data_table_lifecycle_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
