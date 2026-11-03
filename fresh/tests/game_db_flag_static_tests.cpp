#include "pal4/game_db_flag_static.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool okay, const char* message) {
    if (!okay) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IGameDbFlagStaticRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    int registration_result = -7;

    std::uint8_t SetErrorFlag(pal4::GameDbFlagStaticState&,
                              std::uint8_t flag) override {
        events.push_back("flag:" + std::to_string(flag));
        return flag;
    }
    void Reallocate(pal4::GameDbFlagStaticState& state,
                    std::uint32_t length) override {
        events.push_back("allocate:" + std::to_string(length));
        state.data = 0x5001u;
        state.capacity = length;
    }
    void CopyLiteral(std::uint32_t destination,
                     std::string_view text) override {
        events.push_back("copy:" + std::string(text));
        for (std::size_t i = 0u; i < text.size(); ++i)
            bytes[destination + static_cast<std::uint32_t>(i)] =
                static_cast<std::uint8_t>(text[i]);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.push_back("write:" + std::to_string(address));
        bytes[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return bytes.at(address);
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    int RegisterCleanup() override {
        events.push_back("register");
        return registration_result;
    }
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::GameDbFlagStaticState state;
    Check(pal4::GameDbFlagStatic::Register(state, 0xabu, runtime) == -7 &&
              state.initialized_byte == 0xabu && state.data == 0x5001u &&
              state.length == 12u && state.capacity == 12u &&
              runtime.bytes[0x500du] == 0u &&
              runtime.events == std::vector<std::string>{
                  "flag:1", "allocate:12", "copy:GAME_DB_FLAG",
                  "write:20493", "register"},
          "static text construction precedes exit registration");
    const std::string text = "GAME_DB_FLAG";
    for (std::size_t i = 0u; i < text.size(); ++i)
        Check(runtime.bytes[0x5001u + static_cast<std::uint32_t>(i)] ==
                  static_cast<std::uint8_t>(text[i]),
              "static string bytes match IDA literal");

    runtime.events.clear();
    runtime.bytes[0x5000u] = 3u;
    pal4::GameDbFlagStatic::Deinitialize(state, runtime);
    Check(state.cleanup_guard == 1u && state.data == 0u &&
              state.length == 0u && state.capacity == 0u &&
              runtime.bytes[0x5000u] == 2u &&
              runtime.events == std::vector<std::string>{
                  "read:20480", "write:20480"},
          "shared static text decrements its reference");
    runtime.events.clear();
    pal4::GameDbFlagStatic::Deinitialize(state, runtime);
    Check(runtime.events.empty(), "cleanup guard suppresses repeated release");

    for (const auto count : {0u, 255u}) {
        state.cleanup_guard = 0x80u;
        state.data = 0x5001u;
        state.length = state.capacity = 12u;
        runtime.bytes[0x5000u] = static_cast<std::uint8_t>(count);
        runtime.events.clear();
        pal4::GameDbFlagStatic::Deinitialize(state, runtime);
        Check(state.cleanup_guard == 0x81u && state.data == 0u &&
                  runtime.events == std::vector<std::string>{
                      "read:20480", "free:20480"},
              "zero and saturated reference counts free the buffer");
    }
    state.cleanup_guard = 0u;
    state.data = 0u;
    runtime.events.clear();
    pal4::GameDbFlagStatic::Deinitialize(state, runtime);
    Check(state.cleanup_guard == 1u && runtime.events.empty(),
          "null static text requires no release");
    std::cout << "GameDB flag static tests passed\n";
}
