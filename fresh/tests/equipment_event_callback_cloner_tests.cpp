#include "pal4/equipment_event_callback_cloner.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::EquipmentEventCallbackCloneRuntimeStub {
    std::uint32_t target{0x7000u};
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x6008u, 0x11u}, {0x600cu, 0x22u}, {0x6010u, 0x33u}};
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 24u, "callback clone allocation size");
        calls.emplace_back("allocate");
        return target;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        writes.emplace_back(address, value);
    }
};

void TestClone(bool command, std::uint32_t provisional,
               std::uint32_t final_vtable) {
    Runtime runtime;
    const auto result = command ?
        pal4::EquipmentEventCallbackCloner::CloneCommand(0x6000u, runtime) :
        pal4::EquipmentEventCallbackCloner::CloneEvent(0x6000u, runtime);
    Check(result == 0x7000u, "clone returns allocated object");
    Check(runtime.writes ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x7000u, provisional}, {0x7008u, 0x11u},
                  {0x700cu, 0x22u}, {0x7010u, 0x33u},
                  {0x7000u, final_vtable}},
          "clone writes provisional vtable, three fields and final vtable");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "write:28672", "read:24584", "write:28680",
              "read:24588", "write:28684", "read:24592",
              "write:28688", "write:28672"},
          "clone preserves IDA read/write order and leaves +4 untouched");

    runtime = Runtime{};
    runtime.target = 0u;
    const auto failed = command ?
        pal4::EquipmentEventCallbackCloner::CloneCommand(0x6000u, runtime) :
        pal4::EquipmentEventCallbackCloner::CloneEvent(0x6000u, runtime);
    Check(failed == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "allocation failure returns null before source reads");
}

}  // namespace

int main() {
    TestClone(true, 0x843500u, 0x84382cu);
    TestClone(false, 0x8435d0u, 0x8438c4u);
}
