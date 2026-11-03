#include "pal4/combat_event_lifecycle.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEventLifecycleRuntime {
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::string> calls;

    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        writes.emplace_back(address, value);
        calls.emplace_back("write");
    }
    void InitializeEmptyString(std::uint32_t address) override {
        Check(address == 0x2000u + 536u, "router string offset");
        calls.emplace_back("initialize-string");
    }
    std::uint32_t CleanupString(
        std::uint32_t address) override {
        Check(address == 0x2000u + 536u, "cleanup string offset");
        calls.emplace_back("cleanup-string");
        return 0x55u;
    }
};

void ObjectWrites() {
    Runtime runtime;
    Check(pal4::CombatEventLifecycle::InitializeObject(
              0x1000u, 1u, 2u, 3u, 4u, runtime) == 0x1000u &&
              runtime.writes ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x1004u, 1u}, {0x1008u, 2u},
                      {0x1000u, 0x8480ecu}, {0x100cu, 3u},
                      {0x1010u, 4u}},
          "callback writes five words in IDA order and returns self");
}

void RouterLifetime() {
    Runtime runtime;
    Check(pal4::CombatEventLifecycle::InitializeRouter(
              0x2000u, runtime) == 0x2000u &&
              runtime.writes.size() == 134u &&
              runtime.calls.size() == 135u &&
              runtime.calls.back() == "initialize-string",
          "router zeroes 67 pairs before initializing trailing string");
    for (std::size_t index = 0u; index < runtime.writes.size(); ++index) {
        Check(runtime.writes[index].first == 0x2000u + index * 4u &&
                  runtime.writes[index].second == 0u,
              "router word address and zero value");
    }
    Check(pal4::CombatEventLifecycle::CleanupRouter(
              0x2000u, runtime) == 0x55u &&
              runtime.calls.back() == "cleanup-string",
          "router cleanup forwards trailing string result");
}

void ValueCallback(){
    class Runtime final:public pal4::ICombatEventValueInvokeRuntime {
    public:
        std::uint32_t ReadWord(std::uint32_t a)override{
            switch(a){case 0x1004u:return 0x2000u;case 0x1008u:return 0x576440u;case 0x100cu:return 8u;case 0x1010u:return 2u;default:throw std::runtime_error("callback read outside image");}
        }
        std::uint32_t InvokeValue(std::uint32_t owner,std::uint32_t method,std::uint32_t value)override{
            Check(owner==0x2008u && method==0x576440u && value==2u,"member callback preserves this adjustment and result argument");return 73u;
        }
    } runtime;
    Check(pal4::CombatEventLifecycle::InvokeValue(0x1000u,runtime)==73u,"member value callback forwards real operation result");
}

}  // namespace

int main() {
    try {
        ObjectWrites();
        ValueCallback();
        RouterLifetime();
        std::cout << "combat event lifecycle tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
