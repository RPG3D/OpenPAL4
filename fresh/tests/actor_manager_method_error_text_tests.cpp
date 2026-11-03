#include "pal4/actor_manager_method_error_text.h"
#include "pal4/actor_manager_float_clamp.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t object = 0x1000u;
using pal4::ActorManagerMethodErrorText;

struct Runtime final : pal4::IActorManagerMethodErrorRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    bool allocation_success = true;
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
        Check(record == ActorManagerMethodErrorText::kRecord &&
                  length == ActorManagerMethodErrorText::kMessage.size() &&
                  mode == 1u, "global text allocation arguments");
        trace.emplace_back("allocate");
        if (allocation_success)
            words[record + 4u] = 0x2001u;
        return allocation_success;
    }
    void CopyText(std::uint32_t destination,
                  std::string_view text) override {
        Check(destination == 0x2001u, "global text destination");
        copied = text;
        trace.emplace_back("copy");
    }
    void RegisterCleanup(std::uint32_t function) override {
        Check(function == ActorManagerMethodErrorText::kCleanup,
              "atexit callback address");
        trace.emplace_back("atexit");
    }
    std::uint32_t InvokeHandler(std::uint32_t handler,
                                std::uint32_t argument) override {
        Check(handler == 0x3000u && argument == 0x1234u,
              "handler and original argument");
        trace.emplace_back("handler");
        return 0x4567u;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x2000u, "global text allocation base");
        trace.emplace_back("free");
    }
};

void GlobalInitialization() {
    Runtime runtime;
    runtime.bytes[ActorManagerMethodErrorText::kGuard] = 0x80u;
    Check(ActorManagerMethodErrorText::GetOrHandle(
              object, 0x1234u, runtime) ==
              ActorManagerMethodErrorText::kRecord &&
              runtime.bytes[ActorManagerMethodErrorText::kGuard] == 0x81u &&
              runtime.bytes[ActorManagerMethodErrorText::kRecord] == 0x34u &&
              runtime.words[ActorManagerMethodErrorText::kRecord + 8u] ==
                  ActorManagerMethodErrorText::kMessage.size() &&
              runtime.copied == ActorManagerMethodErrorText::kMessage &&
              runtime.trace == std::vector<std::string>{
                  "byte:9931472", "byte:9931488", "word:9931492",
                  "word:9931496", "word:9931500", "allocate", "copy",
                  "word:9931496", "byte:8228", "atexit"},
          "first call initializes global text and registers cleanup");
    runtime.trace.clear();
    runtime.words[object + 8u] = 0x3000u;
    Check(ActorManagerMethodErrorText::GetOrHandle(
              object, 0x1234u, runtime) == 0x4567u &&
              runtime.trace == std::vector<std::string>{"handler"},
          "later call forwards argument through handler without reinit");

    Runtime failure;
    failure.allocation_success = false;
    Check(ActorManagerMethodErrorText::GetOrHandle(
              object, 0x55u, failure) ==
              ActorManagerMethodErrorText::kRecord &&
              failure.words[ActorManagerMethodErrorText::kRecord + 4u] == 0u &&
              failure.trace.back() == "atexit",
          "allocation failure still registers cleanup and returns record");
}

void GlobalCleanup() {
    Runtime owned;
    owned.words[ActorManagerMethodErrorText::kRecord + 4u] = 0x2001u;
    owned.words[ActorManagerMethodErrorText::kRecord + 8u] = 10u;
    owned.words[ActorManagerMethodErrorText::kRecord + 12u] = 12u;
    ActorManagerMethodErrorText::Cleanup(owned);
    Check(owned.trace == std::vector<std::string>{
              "free", "word:9931492", "word:9931496", "word:9931500"},
          "owned global text frees and clears three words");

    Runtime shared;
    shared.words[ActorManagerMethodErrorText::kRecord + 4u] = 0x2001u;
    shared.bytes[0x2000u] = 2u;
    ActorManagerMethodErrorText::Cleanup(shared);
    Check(shared.bytes[0x2000u] == 1u &&
              shared.trace.front() == "byte:8192",
          "shared global text decrements reference byte");

    Runtime sentinel;
    sentinel.words[ActorManagerMethodErrorText::kRecord + 4u] = 0x2001u;
    sentinel.bytes[0x2000u] = 0xffu;
    ActorManagerMethodErrorText::Cleanup(sentinel);
    Check(sentinel.trace.front() == "free",
          "reference sentinel frees global text");
}

void FloatClamp() {
    using pal4::ActorManagerFloatClamp;
    float below = -2.0f;
    Check(ActorManagerFloatClamp::Clamp(below, -1.0f, 1.0f) == 0x0100u &&
              below == -1.0f, "lower bound and less-than status");
    float middle = 0.0f;
    Check(ActorManagerFloatClamp::Clamp(middle, -1.0f, 1.0f) == 0x0100u &&
              middle == 0.0f, "interior status");
    float equal = 1.0f;
    Check(ActorManagerFloatClamp::Clamp(equal, -1.0f, 1.0f) == 0x4000u &&
              equal == 1.0f, "equal upper status");
    float above = 2.0f;
    Check(ActorManagerFloatClamp::Clamp(above, -1.0f, 1.0f) == 0u &&
              above == 1.0f, "upper clamp and greater-than status");
    float nan = std::numeric_limits<float>::quiet_NaN();
    Check(ActorManagerFloatClamp::Clamp(nan, -1.0f, 1.0f) == 0x4100u &&
              std::isnan(nan), "unordered comparison preserves NaN");
}

}  // namespace

int main() {
    try {
        GlobalInitialization();
        GlobalCleanup();
        FloatClamp();
        std::cout << "actor_manager_method_error_text_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
