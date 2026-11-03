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

class Runtime final : public pal4::IMinimapObjectGroupRuntime {
public:
    std::map<std::uint32_t, std::uint32_t> words{
        {0x6000u, 0x4000u}, {0x6008u, 0x7000u},
        {0x600cu, 0x7008u}, {0x7000u, 0x8000u},
        {0x7004u, 0x8100u}};
    std::map<std::uint32_t, std::uint8_t> bytes{
        {0x6004u, 0x42u}};
    std::vector<std::string> events;
    std::uint32_t allocation = 0x9000u;
    bool clear_source_on_allocate = false;

    std::uint32_t ReadDword(std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("byte:" + std::to_string(address));
        return bytes[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        events.emplace_back("put-byte:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes_requested) override {
        events.emplace_back("allocate:" +
                            std::to_string(bytes_requested));
        if (clear_source_on_allocate) {
            words[0x6008u] = 0u;
            words[0x600cu] = 0u;
        }
        return allocation;
    }
};

void TestDeepCopy() {
    Runtime runtime;
    pal4::MinimapObjectCollection::CopyGroup(
        0x6100u, 0x6000u, runtime);
    Check(runtime.words[0x6100u] == 0x4000u &&
              runtime.bytes[0x6104u] == 0x42u &&
              runtime.words[0x6108u] == 0x9000u &&
              runtime.words[0x610cu] == 0x9008u &&
              runtime.words[0x6110u] == 0x9008u &&
              runtime.words[0x9000u] == 0x8000u &&
              runtime.words[0x9004u] == 0x8100u,
          "copy owns a new nested array and retains original icon pointers");
    Check(runtime.events == std::vector<std::string>{
              "read:24576", "write:24832", "byte:24580",
              "put-byte:24836", "read:24584", "read:24588",
              "allocate:8", "write:24840", "read:24588",
              "read:24584", "read:28672",
              "write:36864", "read:28676", "write:36868",
              "write:24844", "write:24848"},
          "header writes, allocation, element copy and end fields are ordered");
}

void TestEmptyAndNullDestination() {
    Runtime runtime;
    pal4::MinimapObjectCollection::CopyGroup(
        0u, 0x6000u, runtime);
    Check(runtime.events.empty(),
          "null destination returns before source reads");
    runtime.words[0x6008u] = 0u;
    runtime.words[0x600cu] = 0u;
    pal4::MinimapObjectCollection::CopyGroup(
        0x6100u, 0x6000u, runtime);
    Check(runtime.words[0x6108u] == 0x9000u &&
              runtime.words[0x610cu] == 0x9000u &&
              runtime.words[0x6110u] == 0x9000u &&
              runtime.events.size() == 11u &&
              runtime.events[5] == "allocate:0",
          "empty nested array still allocates zero bytes and stores result");
    Runtime negative;
    negative.words[0x6008u] = 0x7008u;
    negative.words[0x600cu] = 0x7000u;
    negative.clear_source_on_allocate = true;
    pal4::MinimapObjectCollection::CopyGroup(
        0x6100u, 0x6000u, negative);
    Check(negative.events[6] == "allocate:0" &&
              negative.words[0x610cu] == 0x9000u,
          "negative source element count clamps allocation to zero");
}

}  // namespace

int main() {
    TestDeepCopy();
    TestEmptyAndNullDestination();
    return EXIT_SUCCESS;
}
