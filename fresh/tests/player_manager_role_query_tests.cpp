#include "pal4/player_manager_role_query.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PlayerManagerRoleQueryRuntimeStub {
    std::uint32_t manager{0x5000u};
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;

    std::uint32_t Manager() override {
        events.emplace_back("manager");
        return manager;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        events.push_back("word:" + std::to_string(address));
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.push_back("byte:" + std::to_string(address));
        return bytes[address];
    }
    void SetName(const std::string& text, std::uint32_t declared_length) {
        words[manager + pal4::PlayerManagerRoleQuery::kNameLengthOffset] =
            declared_length;
        words[manager + pal4::PlayerManagerRoleQuery::kNamePointerOffset] =
            0x8000u;
        for (std::uint32_t i = 0u; i < text.size(); ++i)
            bytes[0x8000u + i] = static_cast<std::uint8_t>(text[i]);
    }
};

void CheckName(const std::string& name, std::uint32_t declared_length,
               bool expected, std::uint32_t byte_reads) {
    Runtime runtime;
    runtime.SetName(name, declared_length);
    Check(pal4::PlayerManagerRoleQuery::FirstPlayerIs102(runtime) == expected,
          "first-player name equality follows IDA length and byte comparison");
    Check(runtime.events.size() == 3u + byte_reads &&
          runtime.events[0] == "manager" &&
          runtime.events[1] == "word:" + std::to_string(
              runtime.manager + pal4::PlayerManagerRoleQuery::kNameLengthOffset) &&
          runtime.events[2] == "word:" + std::to_string(
              runtime.manager + pal4::PlayerManagerRoleQuery::kNamePointerOffset),
          "query reads length before data pointer and only needed bytes");
    for (std::uint32_t i = 0u; i < byte_reads; ++i)
        Check(runtime.events[i + 3u] == "byte:" + std::to_string(0x8000u + i),
              "byte comparison advances from the stored text pointer");
}

}  // namespace

int main() {
    Runtime missing;
    missing.manager = 0u;
    Check(!pal4::PlayerManagerRoleQuery::FirstPlayerIs102(missing) &&
          missing.events == std::vector<std::string>{"manager"},
          "null manager returns false without dereferencing a player");

    CheckName("102", 3u, true, 3u);
    CheckName("10", 2u, false, 2u);
    CheckName("102x", 4u, false, 3u);
    CheckName("102", 2u, false, 2u);
    CheckName("", 0u, false, 0u);
    CheckName("1x2", 3u, false, 2u);
    CheckName("202", 3u, false, 1u);
}
