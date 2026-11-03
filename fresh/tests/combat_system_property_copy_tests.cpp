#include "pal4/combat_system_property_copy.h"
#include "pal4/script_combat_config.h"

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

struct Runtime final : pal4::ICombatSystemPropertyCopyRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t system = 0x4000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    [[noreturn]] void ThrowRange() override {
        throw std::out_of_range("text range");
    }
    std::uint8_t DetachAndReset(std::uint32_t record) override {
        calls.emplace_back("detach:" + std::to_string(record));
        return 0u;
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        calls.emplace_back("allocate:" + std::to_string(record) + ":" +
                           std::to_string(length) + ":" +
                           std::to_string(mode));
        words[record + 4u] = 0x9000u + record;
        return true;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        words[record + 8u] = length;
        calls.emplace_back("size:" + std::to_string(record));
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t count) override {
        calls.emplace_back("copy:" + std::to_string(destination) + ":" +
                           std::to_string(source) + ":" +
                           std::to_string(count));
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    std::uint32_t CombatSystemInstance() override {
        calls.emplace_back("system");
        return system;
    }
    void AssignFirstText(std::uint32_t destination,
                         std::uint32_t source) override {
        calls.emplace_back("first:" + std::to_string(destination) + ":" +
                           std::to_string(source));
    }
};

void CopyConfiguration() {
    Runtime runtime;
    constexpr std::uint32_t source = 0x2000u;
    for (std::uint32_t i = 0u; i != 5u; ++i)
        runtime.words[source + i * 4u] = 0x100u + i;
    runtime.words[source + 36u + 4u] = 0x7001u;
    runtime.words[source + 36u + 8u] = 2u;
    runtime.bytes[0x7000u] = 1u;
    runtime.words[source + 52u + 4u] = 0x8001u;
    runtime.words[source + 52u + 8u] = 3u;
    runtime.bytes[0x8000u] = 2u;
    runtime.bytes[source + 68u] = 7u;
    runtime.bytes[source + 69u] = 8u;
    runtime.bytes[source + 70u] = 9u;
    Check(pal4::CombatSystemPropertyCopy::CopyConfiguration(
              source, runtime) == 9u,
          "configuration returns third trailing byte");
    for (std::uint32_t i = 0u; i != 5u; ++i)
        Check(runtime.words[runtime.system + 432u + i * 4u] == 0x100u + i,
              "five header words copied to +432");
    Check(runtime.calls[0] == "system" &&
              runtime.calls[6] == "first:16836:8212" &&
              runtime.words[runtime.system + 468u + 4u] == 0x7001u &&
              runtime.words[runtime.system + 484u + 4u] == 0x8001u &&
              runtime.bytes[0x7000u] == 2u &&
              runtime.bytes[0x8000u] == 3u &&
              runtime.bytes[runtime.system + 500u] == 7u &&
              runtime.bytes[runtime.system + 501u] == 8u &&
              runtime.bytes[runtime.system + 502u] == 9u,
          "text records and trailing flags copied in order");
}

void ForwardingWrappers() {
    Runtime runtime;
    runtime.words[0x3008u] = 2u;
    runtime.words[0x3004u] = 0x7001u;
    runtime.bytes[0x7000u] = 1u;
    Check(pal4::CombatSystemPropertyCopy::AssignFirstText(
              0x3000u, runtime) == 1u &&
              runtime.words[runtime.system + 452u + 4u] == 0x7001u,
          "first wrapper forwards to singleton text slot");
    runtime.words[0x3108u] = 3u;
    runtime.words[0x3104u] = 0x8001u;
    runtime.bytes[0x8000u] = 1u;
    Check(pal4::CombatSystemPropertyCopy::AssignSecondText(
              0x3100u, runtime) == 1u &&
              runtime.words[runtime.system + 468u + 4u] == 0x8001u,
          "second wrapper forwards to singleton text slot");

    runtime.words[0x3208u] = 2u;
    runtime.words[0x3204u] = 0x9001u;
    runtime.bytes[0x9000u] = 1u;
    runtime.calls.clear();
    Check(pal4::ScriptCombatTextThunks::SetBgm(0x3200u, runtime) == 1u &&
              runtime.calls.front() == "system" &&
              runtime.words[runtime.system + 452u + 4u] == 0x9001u,
          "script combat BGM thunk forwards source and result");

    runtime.words[0x3308u] = 3u;
    runtime.words[0x3304u] = 0xa001u;
    runtime.bytes[0xa000u] = 1u;
    runtime.calls.clear();
    Check(pal4::ScriptCombatTextThunks::SetCamera(0x3300u, runtime) == 1u &&
              runtime.calls.front() == "system" &&
              runtime.words[runtime.system + 468u + 4u] == 0xa001u,
          "script combat camera thunk forwards source and result");
}

}  // namespace

int main() {
    try {
        CopyConfiguration();
        ForwardingWrappers();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
