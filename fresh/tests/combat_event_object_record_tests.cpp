#include "pal4/combat_event_object_record.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEventObjectRecordRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return words[address];
    }
    std::uint32_t ApplyModeZero(std::uint32_t event,
                                std::uint32_t target,
                                std::uint32_t value) override {
        calls.push_back("mode-zero " + std::to_string(event) + " " +
                        std::to_string(target) + " " + std::to_string(value));
        return 0x12345678u;
    }
    std::uint32_t ApplyModeOne(std::uint32_t event,
                               std::uint32_t target,
                               std::uint32_t value) override {
        calls.push_back("mode-one " + std::to_string(event) + " " +
                        std::to_string(target) + " " + std::to_string(value));
        return 0xabcdef12u;
    }
    std::uint32_t Query(std::uint32_t event,
                        std::uint32_t value_address,
                        std::uint32_t argument) override {
        calls.push_back("query " + std::to_string(event) + " " +
                        std::to_string(value_address) + " " +
                        std::to_string(argument));
        return 0x1234u;
    }
};

void ProcessModes() {
    Runtime empty;
    Check(pal4::CombatEventObjectRecord::Process(
              1000u, 300u, empty) == 1000u &&
              empty.calls == std::vector<std::string>{"read 1000"},
          "null event returns record without reading mode");

    Runtime zero;
    zero.words = {{1000u, 17u}, {1004u, 0xdeadbeefu}, {1008u, 0u}};
    Check(pal4::CombatEventObjectRecord::Process(
              1000u, 300u, zero) == 0x12345678u &&
              zero.calls == std::vector<std::string>{
                  "read 1000", "read 1008", "read 1004",
                  "mode-zero 17 300 3735928559"},
          "mode zero uses event virtual +0x0c and forwards raw value");

    Runtime one;
    one.words = {{1000u, 17u}, {1004u, 5u}, {1008u, 1u}};
    Check(pal4::CombatEventObjectRecord::Process(
              1000u, 300u, one) == 0xabcdef12u &&
              one.calls.back() == "mode-one 17 300 5",
          "mode one uses event virtual +0x08");

    Runtime other;
    other.words = {{1000u, 17u}, {1008u, 2u}};
    Check(pal4::CombatEventObjectRecord::Process(
              1000u, 300u, other) == 1000u &&
              other.calls == std::vector<std::string>{
                  "read 1000", "read 1008"},
          "other modes return record without value read");
}

void QueryValueAddress() {
    Runtime empty;
    Check(pal4::CombatEventObjectRecord::Query(
              1000u, 300u, empty) == 0u &&
              empty.calls == std::vector<std::string>{"read 1000"},
          "null event returns zero byte");

    Runtime populated;
    populated.words[1000u] = 17u;
    Check(pal4::CombatEventObjectRecord::Query(
              1000u, 300u, populated) == 0x34u &&
              populated.calls == std::vector<std::string>{
                  "read 1000", "query 17 1004 300"},
          "event virtual +0x10 receives the address of value field");
}

}  // namespace

int main() {
    try {
        ProcessModes();
        QueryValueAddress();
        std::cout << "combat event object record tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
