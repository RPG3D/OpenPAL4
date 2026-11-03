#include "pal4/story_data_table.h"

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

struct Runtime final : pal4::IStoryDataTableDestroyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    bool extend_rows = false;

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
    void DestroyStoryText(std::uint32_t text) override {
        calls.push_back("text:" + std::to_string(text));
    }
    void ProcessElements(std::uint32_t elements) override {
        calls.push_back("process:" + std::to_string(elements));
        if (extend_rows) words[elements - 8] = 0x906c;
    }
    void DestroyPropertyRecord(std::uint32_t record) override {
        calls.push_back("property:" + std::to_string(record));
    }
    void DestroyStoryRecord(std::uint32_t record) override {
        calls.push_back("record:" + std::to_string(record));
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint32_t DestroyBase(std::uint32_t table) override {
        calls.push_back("base:" + std::to_string(table));
        return 0x7171;
    }
};

void CheckPopulated() {
    Runtime runtime;
    constexpr std::uint32_t table = 0x7000;
    runtime.words[table + 96] = 0x9000;
    runtime.words[table + 100] = 0x9048;
    runtime.words[table + 112] = 0xa000;
    runtime.words[table + 116] = 0xa078;
    runtime.words[table + 128] = 0xb000;
    runtime.extend_rows = true;
    Check(pal4::StoryDataTable::Destroy(table, runtime) == 0x7171,
          "base destructor result is forwarded");
    const std::vector<std::string> expected = {
        "write:28672:8669004", "clear:28672",
        "text:36884", "text:36868",
        "text:36920", "text:36904",
        "write:28772:36864", "process:28780",
        "free:45056",
        "write:28800:0", "write:28804:0", "write:28808:0",
        "property:40960", "property:41020", "free:40960",
        "write:28784:0", "write:28788:0", "write:28792:0",
        "record:36864", "record:36900", "record:36936",
        "free:36864",
        "write:28768:0", "write:28772:0", "write:28776:0",
        "base:28672"};
    Check(runtime.calls == expected,
          "two text fields are released in reverse order, then elements, properties and refreshed row range");
    for (const auto offset : {96u, 100u, 104u, 112u, 116u, 120u,
                              128u, 132u, 136u})
        Check(runtime.words[table + offset] == 0,
              "all three storage pointer triplets are cleared");
}

void CheckEmpty() {
    Runtime runtime;
    constexpr std::uint32_t table = 0x7000;
    Check(pal4::StoryDataTable::Destroy(table, runtime) == 0x7171,
          "empty table still destroys base");
    for (const auto& call : runtime.calls)
        Check(call.rfind("text:", 0) != 0 &&
                  call.rfind("record:", 0) != 0 &&
                  call.rfind("property:", 0) != 0,
              "empty ranges have no record destruction");
    Check(runtime.calls.back() == "base:28672",
          "base destruction remains final for empty table");
}

}  // namespace

int main() {
    try {
        CheckPopulated();
        CheckEmpty();
        std::cout << "story_data_table_destroy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
