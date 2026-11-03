#include "pal4/actor_manager_method_error_text.h"

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

using ErrorText = pal4::ActorManagerPlayerMethodErrorText;
constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kBuffer = 0x5001u;

struct Runtime final : pal4::IActorManagerMethodErrorRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    bool allocation_succeeds = true;
    std::string copied;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        trace.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("word:" + std::to_string(address));
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(record == ErrorText::kRecord &&
                  length == pal4::ActorManagerMethodErrorText::kMessage.size()
                  && mode == 1u,
              "player error text allocation arguments");
        trace.emplace_back("allocate");
        if (allocation_succeeds) words[record + 4u] = kBuffer;
        return allocation_succeeds;
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        Check(destination == kBuffer, "player error text buffer");
        copied = text;
        trace.emplace_back("copy");
    }
    void RegisterCleanup(std::uint32_t function) override {
        Check(function == ErrorText::kCleanup, "player cleanup address");
        trace.emplace_back("atexit");
    }
    std::uint32_t InvokeHandler(std::uint32_t handler,
                                std::uint32_t argument) override {
        Check(handler == 0x3300u && argument == 0x1234u,
              "player handler arguments");
        trace.emplace_back("handler");
        return 0x7788u;
    }
    void Free(std::uint32_t address) override {
        Check(address == kBuffer - 1u, "player text header");
        trace.emplace_back("free");
    }
};

}  // namespace

int main() {
    try {
        Runtime first;
        first.bytes[ErrorText::kGuard] = 0x80u;
        Check(ErrorText::GetOrHandle(kObject, 0x1234u, first) ==
                  ErrorText::kRecord &&
                  first.bytes[ErrorText::kGuard] == 0x81u &&
                  first.bytes[ErrorText::kRecord] == 0x34u &&
                  first.words[ErrorText::kRecord + 4u] == kBuffer &&
                  first.words[ErrorText::kRecord + 8u] ==
                      pal4::ActorManagerMethodErrorText::kMessage.size() &&
                  first.copied ==
                      pal4::ActorManagerMethodErrorText::kMessage &&
                  first.trace.back() == "atexit",
              "first call builds global text and registers cleanup");
        first.trace.clear();
        first.words[kObject + 8u] = 0x3300u;
        Check(ErrorText::GetOrHandle(kObject, 0x1234u, first) == 0x7788u &&
                  first.trace == std::vector<std::string>{"handler"},
              "later call forwards original argument without rebuilding");

        Runtime failed;
        failed.allocation_succeeds = false;
        Check(ErrorText::GetOrHandle(kObject, 0u, failed) ==
                  ErrorText::kRecord &&
                  failed.words[ErrorText::kRecord + 4u] == 0u &&
                  failed.trace.back() == "atexit",
              "allocation failure still registers cleanup");

        Runtime owned;
        owned.words[ErrorText::kRecord + 4u] = kBuffer;
        owned.words[ErrorText::kRecord + 8u] = 35u;
        ErrorText::Cleanup(owned);
        Check(owned.trace == std::vector<std::string>{
                  "free", "word:9931428", "word:9931432",
                  "word:9931436"} &&
                  owned.words[ErrorText::kRecord + 4u] == 0u &&
                  owned.words[ErrorText::kRecord + 8u] == 0u &&
                  owned.words[ErrorText::kRecord + 12u] == 0u,
              "owned fallback frees and clears pointer, size and capacity");

        Runtime shared;
        shared.words[ErrorText::kRecord + 4u] = kBuffer;
        shared.bytes[kBuffer - 1u] = 2u;
        ErrorText::Cleanup(shared);
        Check(shared.bytes[kBuffer - 1u] == 1u &&
                  shared.trace.front() == "byte:20480",
              "shared fallback decrements reference count");

        Runtime sentinel;
        sentinel.words[ErrorText::kRecord + 4u] = kBuffer;
        sentinel.bytes[kBuffer - 1u] = 0xffu;
        ErrorText::Cleanup(sentinel);
        Check(sentinel.trace.front() == "free",
              "sentinel fallback is freed");
        std::cout << "actor_manager_player_method_error_text_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
