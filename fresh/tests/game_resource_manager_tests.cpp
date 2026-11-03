#include "support/rw_dict_schema_fake.h"
#include "pal4/game_resource_manager.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Events {
    std::vector<std::string> calls;
};

class Memory final : public pal4::IGameConfigMemory {
public:
    explicit Memory(Events& events) : events_(events) {}
    std::uint32_t Free(std::uint32_t address) override {
        events_.calls.push_back("memory:" + std::to_string(address));
        return address;
    }
private:
    Events& events_;
};

class SchemaRuntime final : public pal4::test::RwDictSchemaFake {
public:
    explicit SchemaRuntime(Events& events) : events_(events) {}
    void DestroyList(std::uint32_t address) override {
        events_.calls.push_back("list:" + std::to_string(address));
    }
    void DestroyDict(std::uint32_t address) override {
        events_.calls.push_back("dict:" + std::to_string(address));
    }
    std::uint32_t SetCurrentDict(std::uint32_t value) override {
        events_.calls.push_back("current:" + std::to_string(value));
        return 0;
    }
private:
    Events& events_;
};

void ConstructorFields() {
    pal4::GameResourceManager manager;
    manager.words.fill(0xdeadbeefu);
    Check(&manager.Construct() == &manager, "constructor returns object");
    Check(manager.words[0] == 0x84b8c8u, "vtable marker");
    for (std::size_t i = 1; i <= 13; ++i)
        Check(manager.words[i] == (i == 10 ? 1u : 0u), "header fields");
    for (std::size_t i = 14; i <= 28; ++i)
        Check(manager.words[i] == 0, "ConfigData subobject initialized");
    Check(manager.words[37] == 0 && manager.words[38] == 0 &&
          manager.words[39] == 0x3f800000u, "dictionary schema subobject");
    for (std::size_t i = 40; i <= 45; ++i)
        Check(manager.words[i] == 0, "cleared material slots");
    for (std::size_t i = 46; i <= 51; ++i)
        Check(manager.words[i] == 0x3f800000u, "unit float slots");
    for (std::size_t i = 73; i <= 75; ++i)
        Check(manager.words[i] == 0, "tail pointer slots");
    for (std::size_t i = 76; i <= 91; ++i) {
        const auto local = i - 76;
        const auto expected = local == 6 || local == 7 ? 0x3f800000u :
                              local == 13 ? 3u : 0u;
        Check(manager.words[i] == expected, "nested game-config defaults");
    }
    Check(manager.words[29] == 0xdeadbeefu &&
          manager.words[72] == 0xdeadbeefu,
          "constructor preserves untouched words");
}

void DestructorOrder() {
    pal4::GameResourceManager manager;
    (void)manager.Construct();
    manager.words[1] = 11;
    manager.words[3] = 13;
    manager.words[4] = 14;
    manager.words[37] = 37;
    manager.words[38] = 38;
    manager.words[76 + 9] = 89;
    manager.words[76 + 12] = 92;
    manager.words[76 + 14] = 94;
    manager.words[76 + 15] = 95;
    Events events;
    Memory memory(events);
    SchemaRuntime schema(events);
    manager.Destruct(memory, schema);
    Check(events.calls == std::vector<std::string>{
              "memory:11", "memory:13", "memory:14", "memory:89",
              "memory:92", "list:38", "dict:37", "current:0"},
          "destructor calls nested cleanup in IDA order");
    Check(manager.words[1] == 0 && manager.words[2] == 0 &&
          manager.words[3] == 0 && manager.words[4] == 0 &&
          manager.words[37] == 0 && manager.words[38] == 0 &&
          manager.words[76 + 9] == 0 && manager.words[76 + 12] == 0 &&
          manager.words[76 + 14] == 0 && manager.words[76 + 15] == 0,
          "owned pointer slots cleared");
    events.calls.clear();
    manager.Destruct(memory, schema);
    Check(events.calls == std::vector<std::string>{"current:0"},
          "empty destructor only resets current dictionary");
}

}  // namespace

int main() {
    ConstructorFields();
    DestructorOrder();
}
