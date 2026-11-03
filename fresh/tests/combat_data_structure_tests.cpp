#include "pal4/combat_data_structure.h"

#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatDataStructureRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t allocated = 0x8000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address) + ":" +
                           std::to_string(value));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x24u, "data node allocation size");
        calls.emplace_back("allocate");
        return allocated;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return 0xaaaau;
    }
    void CleanupEventData(std::uint32_t payload) override {
        calls.emplace_back("payload:" + std::to_string(payload));
    }
    std::uint32_t DestroyVirtual(std::uint32_t object,
                                 std::uint32_t flag) override {
        Check(flag == 1u, "range deleting destructor flag");
        calls.emplace_back("destroy:" + std::to_string(object));
        return object + 1u;
    }
};

void Construction() {
    Runtime runtime;
    Check(pal4::CombatDataStructure::Construct(
              0u, 0u, runtime) == 0x8000u &&
              runtime.words[0x8000u] == 0x8000u &&
              runtime.words[0x8004u] == 0x8000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "write:32768:32768", "write:32772:32768"},
          "null arguments make both links self-referential");
    runtime.calls.clear();
    Check(pal4::CombatDataStructure::Construct(
              0x1111u, 0x2222u, runtime) == 0x8000u &&
              runtime.words[0x8000u] == 0x1111u &&
              runtime.words[0x8004u] == 0x2222u,
          "explicit links replace defaults");
    runtime.allocated = 0u;
    bool raised = false;
    try {
        static_cast<void>(pal4::CombatDataStructure::Construct(
            0u, 0u, runtime));
    } catch (const std::bad_alloc&) {
        raised = true;
    }
    Check(raised, "native null-dereference allocation becomes explicit failure");
}

void EventCollection() {
    Runtime empty;
    empty.words[0x1004u] = 0x2000u;
    empty.words[0x2000u] = 0x2000u;
    empty.words[0x2004u] = 0x2000u;
    Check(pal4::CombatDataStructure::CleanupEventCollection(
              0x1000u, empty) == 0u &&
              empty.calls == std::vector<std::string>{
                  "free:8192", "write:4100:0", "write:4104:0"},
          "empty circular list frees sentinel and clears collection");

    Runtime two;
    two.words[0x1004u] = 0x2000u;
    two.words[0x1008u] = 2u;
    two.words[0x2000u] = 0x3000u;
    two.words[0x2004u] = 0x4000u;
    two.words[0x3000u] = 0x4000u;
    two.words[0x3004u] = 0x2000u;
    two.words[0x4000u] = 0x2000u;
    two.words[0x4004u] = 0x3000u;
    Check(pal4::CombatDataStructure::CleanupEventCollection(
              0x1000u, two) == 0u &&
              two.calls == std::vector<std::string>{
                  "write:8192:16384", "write:16388:8192",
                  "payload:12296", "free:12288", "write:4104:1",
                  "write:8192:8192", "write:8196:8192",
                  "payload:16392", "free:16384", "write:4104:0",
                  "free:8192", "write:4100:0", "write:4104:0"},
          "two nodes unlink, clean payload and free in forward order");

    Runtime invalid;
    bool raised = false;
    try {
        static_cast<void>(pal4::CombatDataStructure::CleanupEventCollection(
            0x1000u, invalid));
    } catch (const std::invalid_argument&) {
        raised = true;
    }
    Check(raised, "null sentinel is explicit invalid input");
}

void Range() {
    Runtime runtime;
    runtime.bytes[0x5000u] = 0u;
    runtime.bytes[0x5008u] = 1u;
    runtime.words[0x500cu] = 0u;
    runtime.bytes[0x5010u] = 1u;
    runtime.words[0x5014u] = 0x6000u;
    runtime.bytes[0x5018u] = 0xffu;
    runtime.words[0x501cu] = 0x7000u;
    Check(pal4::CombatDataStructure::DestroyRange(
              0x5000u, 0x5020u, runtime) == 0x7001u &&
              runtime.calls == std::vector<std::string>{
                  "destroy:24576", "destroy:28672"},
          "range skips empty flags and pointers and returns last destroy");
    runtime.calls.clear();
    Check(pal4::CombatDataStructure::DestroyRange(
              0x5000u, 0x5000u, runtime) == 0u &&
              runtime.calls.empty(),
          "empty range uses defined zero return");
}

}  // namespace

int main() {
    try {
        Construction();
        EventCollection();
        Range();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
