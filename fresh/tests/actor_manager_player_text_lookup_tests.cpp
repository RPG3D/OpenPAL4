#include "pal4/actor_manager_player_text_lookup.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using Lookup = pal4::ActorManagerPlayerTextLookup;

struct Runtime final : pal4::IActorManagerPlayerTextRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    bool allocation_succeeds = true;
    std::uint32_t buffer = 0x5001u;

    std::uint8_t ReadByte(std::uint32_t address) override {
        trace.emplace_back("read_byte");
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        trace.emplace_back("write_byte");
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        trace.emplace_back("read_word");
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        trace.emplace_back("write_word");
        words[address] = value;
    }
    void SetErrorFlag(std::uint32_t record, std::uint8_t value) override {
        Check(record == Lookup::kFallback && value == 1u,
              "fallback flag arguments");
        trace.emplace_back("flag");
    }
    void DetachAndReset(std::uint32_t) override {
        throw std::runtime_error("unexpected detach");
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(record == Lookup::kFallback && length == 5u && mode == 1u,
              "fallback allocation arguments");
        trace.emplace_back("allocate");
        if (allocation_succeeds) words[record + 4u] = buffer;
        return allocation_succeeds;
    }
    void SetTextSize(std::uint32_t, std::uint32_t) override {
        throw std::runtime_error("unexpected size helper");
    }
    void CopyBytes(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {
        throw std::runtime_error("unexpected copy helper");
    }
    void WriteLiteral(std::uint32_t destination,
                      std::string_view text) override {
        Check(destination == buffer && text == "ERROR", "error text");
        trace.emplace_back("literal");
    }
    void RegisterCleanup(std::uint32_t function) override {
        Check(function == Lookup::kCleanupFunction, "cleanup address");
        trace.emplace_back("atexit");
    }
    void Free(std::uint32_t address) override {
        Check(address == buffer - 1u, "buffer header address");
        trace.emplace_back("free");
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        for (std::int32_t index = 0; index < 4; ++index)
            Check(Lookup::Record(0x1000u, index, runtime) ==
                      0x1000u + static_cast<std::uint32_t>(index) * 16u &&
                      runtime.trace.empty(),
                  "valid record address does not initialize fallback");

        runtime.bytes[Lookup::kGuard] = 0x80u;
        Check(Lookup::Record(0x1000u, -1, runtime) == Lookup::kFallback &&
                  runtime.bytes[Lookup::kGuard] == 0x81u &&
                  runtime.bytes[Lookup::kFallback] == 0xffu &&
                  runtime.words[Lookup::kFallback + 4u] == runtime.buffer &&
                  runtime.words[Lookup::kFallback + 8u] == 5u &&
                  runtime.bytes[runtime.buffer + 5u] == 0u &&
                  runtime.trace.back() == "atexit",
              "negative index initializes the error record once");
        runtime.trace.clear();
        Check(Lookup::Record(0x2000u, 4, runtime) == Lookup::kFallback &&
                  runtime.trace == std::vector<std::string>{"read_byte"} &&
                  runtime.bytes[Lookup::kFallback] == 0xffu,
              "subsequent invalid index preserves first flag and text");

        runtime.trace.clear();
        runtime.bytes[runtime.buffer - 1u] = 2u;
        Lookup::Cleanup(runtime);
        Check(runtime.bytes[runtime.buffer - 1u] == 1u &&
                  runtime.words[Lookup::kFallback + 4u] == 0u &&
                  runtime.words[Lookup::kFallback + 8u] == 0u &&
                  runtime.words[Lookup::kFallback + 12u] == 0u &&
                  runtime.bytes[Lookup::kGuard] == 0x81u,
              "shared cleanup decrements and clears without resetting guard");

        Runtime owned;
        owned.words[Lookup::kFallback + 4u] = owned.buffer;
        Lookup::Cleanup(owned);
        Check(owned.trace[2] == "free", "zero-reference buffer is freed");
        Runtime sentinel;
        sentinel.words[Lookup::kFallback + 4u] = sentinel.buffer;
        sentinel.bytes[sentinel.buffer - 1u] = 0xffu;
        Lookup::Cleanup(sentinel);
        Check(sentinel.trace[2] == "free", "sentinel buffer is freed");

        Runtime failed;
        failed.allocation_succeeds = false;
        Check(Lookup::Record(0x1000u, 4, failed) == Lookup::kFallback &&
                  failed.words[Lookup::kFallback + 4u] == 0u &&
                  failed.trace.back() == "atexit",
              "allocation failure still registers cleanup");
        std::cout << "actor_manager_player_text_lookup_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
