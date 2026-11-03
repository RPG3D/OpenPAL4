#include "pal4/combat_property_writer.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatPropertyWriterRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void SetString(std::uint32_t node, std::uint32_t key,
                   std::uint32_t value) override {
        calls.emplace_back("string:" + std::to_string(node) + ":" +
                           std::to_string(key) + ":" +
                           std::to_string(value));
    }
    std::uint8_t WriteInt(std::uint32_t node, std::uint32_t key,
                          std::uint32_t value) override {
        calls.emplace_back("int:" + std::to_string(node) + ":" +
                           std::to_string(key) + ":" +
                           std::to_string(value));
        return static_cast<std::uint8_t>(value);
    }
    std::uint32_t CreateNode(std::uint32_t document,
                             std::uint32_t name) override {
        calls.emplace_back("node:" + std::to_string(document) + ":" +
                           std::to_string(name));
        return 0x3000u;
    }
    std::uint32_t CombatSystemInstance() override {
        calls.emplace_back("system");
        return 0x4000u;
    }
    std::uint32_t SetProperty110(std::uint32_t system,
                                 std::uint32_t value) override {
        calls.emplace_back("property:" + std::to_string(system) + ":" +
                           std::to_string(value));
        return value + 1u;
    }
};

void CombatProperties() {
    Runtime runtime;
    runtime.words[0x8fa268u] = 0x101u;
    runtime.words[0x8fa258u] = 0x102u;
    runtime.words[0x8fa248u] = 0x103u;
    Check(pal4::CombatPropertyWriter::SetCombatProperties(
              0x2000u, 0x501u, 0x502u, 7u, 0x88u, runtime) == 0x89u,
          "combat property result");
    Check(runtime.calls == std::vector<std::string>{
              "read:9413224", "string:8192:257:1281",
              "read:9413208", "string:8192:258:1282",
              "read:9413192", "int:8192:259:7",
              "system", "property:16384:136"},
          "combat property keys, values and call order");

    Runtime empty;
    static_cast<void>(pal4::CombatPropertyWriter::SetCombatProperties(
        1u, 2u, 3u, 4u, 5u, empty));
    Check(empty.calls[1] == "string:1:8657252:2" &&
              empty.calls[3] == "string:1:8657252:3" &&
              empty.calls[5] == "int:1:8657252:4",
          "missing global key falls back to empty string");
}

void MonsterProperties() {
    Runtime runtime;
    runtime.words[0x8fa228u] = 0x201u;
    runtime.words[0x8fa218u] = 0x202u;
    runtime.words[0x8fa204u] = 0x203u;
    runtime.words[0x8fa1f4u] = 0x204u;
    Check(pal4::CombatPropertyWriter::SetMonsterProperties(
              0x5000u, 11u, 22u, 33u, runtime) == 33u,
          "monster property result comes from last integer write");
    Check(runtime.calls == std::vector<std::string>{
              "read:9413160", "node:20480:513",
              "read:9413144", "int:12288:514:11",
              "read:9413124", "int:12288:515:22",
              "read:9413108", "int:12288:516:33"},
          "monster node and three attributes in IDA order");

    Runtime empty;
    static_cast<void>(pal4::CombatPropertyWriter::SetMonsterProperties(
        1u, 2u, 3u, 4u, empty));
    Check(empty.calls[1] == "node:1:8657252" &&
              empty.calls[3] == "int:12288:8657252:2" &&
              empty.calls[5] == "int:12288:8657252:3" &&
              empty.calls[7] == "int:12288:8657252:4",
          "missing monster keys all fall back to empty string");
}

}  // namespace

int main() {
    try {
        CombatProperties();
        MonsterProperties();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
