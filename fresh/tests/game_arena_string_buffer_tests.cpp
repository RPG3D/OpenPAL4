#include "pal4/game_arena_string_buffer.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameArenaStringBufferRuntime {
    std::uint32_t source_offset = 8u;
    std::uint32_t length = 3u;
    std::uint32_t data = 0u;
    std::uint32_t initial_data = 0u;
    std::uint32_t capacity = 0u;
    std::uint8_t prefix = 0u;
    std::uint32_t stored_length = 0u;
    std::vector<std::string> calls;

    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x1000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x1000u + source_offset) {
            calls.emplace_back("source");
            return 0x3000u;
        }
        if (address == 0x2004u) {
            calls.emplace_back("data");
            return data;
        }
        if (address != 0x200cu)
            throw std::runtime_error("unexpected word address " +
                                     std::to_string(address));
        calls.emplace_back("capacity");
        return capacity;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == data - 1u, "buffer prefix address");
        calls.emplace_back("prefix");
        return prefix;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        Check(address == 0x2008u && value == length,
              "buffer stored length");
        calls.emplace_back("size");
        stored_length = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        if (address == data - 1u) {
            Check(value == static_cast<std::uint8_t>(prefix - 1u),
                  "prefix decremented exactly once");
            calls.emplace_back("decrement prefix");
            prefix = value;
            return;
        }
        Check(address == data + length && value == 0u,
              "buffer NUL terminator");
        calls.emplace_back("terminate");
    }
    void InitializeBuffer(std::uint32_t destination) override {
        Check(destination == 0x2000u, "buffer destination");
        calls.emplace_back("initialize");
        data = initial_data;
        stored_length = 0u;
    }
    std::uint32_t StringLength(std::uint32_t source) override {
        Check(source == 0x3000u, "source string pointer");
        calls.emplace_back("length");
        return length;
    }
    void SetErrorFlag(std::uint32_t destination,
                      std::uint8_t value) override {
        Check(destination == 0x2000u, "flag destination");
        calls.emplace_back("flag:" + std::to_string(value));
    }
    void Reallocate(std::uint32_t destination,
                    std::uint32_t requested_length) override {
        Check(destination == 0x2000u && requested_length == length,
              "reallocation arguments");
        calls.emplace_back("reallocate");
        data = 0x5000u;
        capacity = length;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t copy_length) override {
        Check(destination == data && source == 0x3000u &&
                  copy_length == length, "buffer copy operands");
        calls.emplace_back("copy");
    }
};

void BasicBranches() {
    for (auto second : {false, true}) {
        Runtime copied;
        copied.source_offset = second ? 0xe0u : 8u;
        const auto result = second ?
            pal4::GameArenaStringBuffer::Second(0x2000u, copied) :
            pal4::GameArenaStringBuffer::First(0x2000u, copied);
        Check(result == 0x2000u && copied.stored_length == 3u &&
                  copied.calls == std::vector<std::string>{
                      "arena", "source", "initialize", "length", "data",
                      "capacity", "flag:1", "reallocate", "data", "copy",
                      "size", "data", "terminate"},
              "each arena field allocates and copies a nonempty source");
    }
    Runtime empty;
    empty.length = 0u;
    Check(pal4::GameArenaStringBuffer::First(0x2000u, empty) == 0x2000u &&
              empty.calls == std::vector<std::string>{
                  "arena", "source", "initialize", "length", "data",
                  "flag:1"},
          "empty arena string sets flag and returns without allocation");
}

void PrefixBranches() {
    Runtime empty_prefix;
    empty_prefix.length = 0u;
    empty_prefix.initial_data = 0x4100u;
    empty_prefix.prefix = 2u;
    Check(pal4::GameArenaStringBuffer::First(
              0x2000u, empty_prefix) == 0x2000u &&
              empty_prefix.prefix == 1u &&
              empty_prefix.calls == std::vector<std::string>{
                  "arena", "source", "initialize", "length", "data",
                  "prefix", "decrement prefix", "flag:0"},
          "live prefix and empty source decrement reference byte");
    Runtime nonempty_prefix;
    nonempty_prefix.initial_data = 0x4100u;
    nonempty_prefix.prefix = 2u;
    Check(pal4::GameArenaStringBuffer::First(
              0x2000u, nonempty_prefix) == 0x2000u &&
              nonempty_prefix.calls == std::vector<std::string>{
                  "arena", "source", "initialize", "length", "data",
                  "prefix", "reallocate", "data", "copy", "size", "data",
                  "terminate"},
          "live prefix and nonempty source reallocate without flag write");
    for (auto prefix : {0u, 255u}) {
        Runtime reusable;
        reusable.source_offset = 0xe0u;
        reusable.initial_data = 0x4100u;
        reusable.capacity = 31u;
        reusable.prefix = static_cast<std::uint8_t>(prefix);
        Check(pal4::GameArenaStringBuffer::Second(
                  0x2000u, reusable) == 0x2000u &&
                  reusable.calls == std::vector<std::string>{
                      "arena", "source", "initialize", "length", "data",
                      "prefix", "capacity", "data", "copy", "size", "data",
                      "terminate"},
              "zero and sentinel prefixes reuse sufficient buffer");
    }
}

void CapacityAndLengthBranches() {
    for (auto capacity : {2u, 32u}) {
        Runtime runtime;
        runtime.initial_data = 0x4100u;
        runtime.capacity = capacity;
        Check(pal4::GameArenaStringBuffer::First(
                  0x2000u, runtime) == 0x2000u &&
                  runtime.calls == std::vector<std::string>{
                      "arena", "source", "initialize", "length", "data",
                      "prefix", "capacity", "flag:1", "reallocate", "data",
                      "copy", "size", "data", "terminate"},
              "short or over-31 capacity triggers reallocation");
    }
    Runtime too_long;
    too_long.length = 0xfffffffeu;
    try {
        (void)pal4::GameArenaStringBuffer::First(0x2000u, too_long);
        throw std::runtime_error("expected length error");
    } catch (const std::length_error&) {
        Check(too_long.calls == std::vector<std::string>{
                  "arena", "source", "initialize", "length"},
              "length error occurs before buffer field reads");
    }
}

}  // namespace

int main() {
    try {
        BasicBranches();
        PrefixBranches();
        CapacityAndLengthBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
