#include "pal4/minimap_object_collection.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IMinimapObjectArrayRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words{
        {0x6004u, 0x9000u}, {0x6008u, 0x900cu},
        {0x6104u, 0x7000u}, {0x6108u, 0x7004u},
        {0x610cu, 0x7008u},
        {0x9000u, 0x8000u}, {0x9004u, 0x8100u},
        {0x9008u, 0x8200u}, {0x7000u, 0x4000u}};
    std::vector<std::string> events;

    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.emplace_back("allocate:" + std::to_string(bytes));
        return 0xa000u;
    }
    void Free(std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }
};

void TestSelfAndOverwrite() {
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::AssignNestedArray(
              0x6100u, 0x6100u, runtime) == 0x6100u &&
              runtime.events.empty(),
          "self-assignment reads no fields");
    runtime.words[0x6008u] = 0x9008u;
    runtime.words[0x6108u] = 0x700cu;
    Check(pal4::MinimapObjectCollection::AssignNestedArray(
              0x6100u, 0x6000u, runtime) == 0x6100u &&
              runtime.words[0x7000u] == 0x8000u &&
              runtime.words[0x7004u] == 0x8100u &&
              runtime.words[0x6108u] == 0x7008u &&
              runtime.words[0x610cu] == 0x7008u,
          "shorter source overwrites prefix and preserves capacity");
    Check(runtime.events == std::vector<std::string>{
              "read:24580", "read:24584", "read:24836",
              "read:24840", "read:36864", "write:28672",
              "read:36868", "write:28676", "write:24840"},
          "overwrite does not allocate or free");
    runtime.events.clear();
    runtime.words[0x6004u] = 0u;
    runtime.words[0x6008u] = 0u;
    Check(pal4::MinimapObjectCollection::AssignNestedArray(
              0x6100u, 0x6000u, runtime) == 0x6100u &&
              runtime.words[0x6108u] == 0x7000u &&
              runtime.events.size() == 5u,
          "empty source only contracts target end");
}

void TestReallocate() {
    Runtime runtime;
    Check(pal4::MinimapObjectCollection::AssignNestedArray(
              0x6100u, 0x6000u, runtime) == 0x6100u &&
              runtime.words[0x6104u] == 0xa000u &&
              runtime.words[0x6108u] == 0xa00cu &&
              runtime.words[0x610cu] == 0xa00cu &&
              runtime.words[0xa000u] == 0x8000u &&
              runtime.words[0xa004u] == 0x8100u &&
              runtime.words[0xa008u] == 0x8200u,
          "source beyond capacity frees and allocates exact pointer count");
    Check(runtime.events[5] == "free:28672" &&
              runtime.events[6] == "allocate:12" &&
              runtime.events.back() == "write:24840",
          "free precedes allocation and end follows capacity write");
}

void TestAppendWithinCapacity() {
    Runtime runtime;
    runtime.words[0x610cu] = 0x7010u;
    Check(pal4::MinimapObjectCollection::AssignNestedArray(
              0x6100u, 0x6000u, runtime) == 0x6100u &&
              runtime.words[0x7000u] == 0x8000u &&
              runtime.words[0x7004u] == 0x8100u &&
              runtime.words[0x7008u] == 0x8200u &&
              runtime.words[0x6108u] == 0x700cu &&
              runtime.words[0x610cu] == 0x7010u,
          "available capacity appends remaining pointers in place");
    for (const auto& event : runtime.events) {
        Check(event.find("allocate:") != 0u &&
                  event.find("free:") != 0u,
              "in-place append avoids allocation and free");
    }
}

}  // namespace

int main() {
    TestSelfAndOverwrite();
    TestReallocate();
    TestAppendWithinCapacity();
    return EXIT_SUCCESS;
}
