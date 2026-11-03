#include "pal4/actor_manager_player_text_property.h"

#include <algorithm>
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

using Property = pal4::ActorManagerPlayerTextProperty;
constexpr std::uint32_t actor = 0x1000u;
constexpr std::uint32_t source = 0x4000u;

struct Runtime final : pal4::IActorManagerPlayerTextRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> freed;
    std::uint32_t allocation = 0x8001u;
    std::uint32_t allocated_length = 0u;
    std::uint8_t allocated_mode = 0u;
    bool allocation_succeeds = true;
    bool detach_zeros_size = false;
    std::uint32_t self_size_reads = 0u;
    bool shrink_second_self_read = false;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        trace.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (shrink_second_self_read && address == actor + 1120u &&
            ++self_size_reads == 2u)
            return 1u;
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("word:" + std::to_string(address));
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t value) override {
        Check(value == 1u, "shared assignment detaches old text");
        trace.emplace_back("set_error");
        words[record + 4u] = 0u;
        words[record + 8u] = 0u;
        words[record + 12u] = 0u;
    }
    void DetachAndReset(std::uint32_t record) override {
        trace.emplace_back("detach");
        if (detach_zeros_size) {
            words[record + 4u] = 0u;
            words[record + 8u] = 0u;
            words[record + 12u] = 0u;
        }
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        allocated_length = length;
        allocated_mode = mode;
        trace.emplace_back("allocate:" + std::to_string(mode));
        if (!allocation_succeeds) return false;
        words[record + 4u] = allocation;
        return true;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        trace.emplace_back("size");
        words[record + 8u] = length;
        bytes[words[record + 4u] + length] = 0u;
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t from,
                   std::uint32_t count) override {
        trace.emplace_back("copy:" + std::to_string(count));
        for (std::uint32_t i = 0u; i < count; ++i)
            bytes[destination + i] = bytes[from + i];
    }
    void WriteLiteral(std::uint32_t destination,
                      std::string_view text) override {
        trace.emplace_back("literal");
        for (std::uint32_t i = 0u; i < text.size(); ++i)
            bytes[destination + i] = static_cast<std::uint8_t>(text[i]);
    }
    void RegisterCleanup(std::uint32_t function) override {
        Check(function == Property::kCleanupFunction,
              "fallback registers the correct cleanup entry");
        trace.emplace_back("atexit");
    }
    void Free(std::uint32_t address) override {
        trace.emplace_back("free");
        freed.push_back(address);
    }
};

void InvalidIndexAndCleanup() {
    Runtime runtime;
    runtime.bytes[Property::kGuard] = 0x80u;
    runtime.allocation = 0x9001u;
    Check(Property::Assign(actor, -1, source, runtime) ==
              Property::kFallback &&
              runtime.bytes.at(Property::kGuard) == 0x81u &&
              runtime.bytes.at(Property::kFallback) == 0xffu &&
              runtime.words.at(Property::kFallback + 4u) == 0x9001u &&
              runtime.words.at(Property::kFallback + 8u) == 5u &&
              runtime.bytes.at(0x9001u) == 'E' &&
              runtime.bytes.at(0x9005u) == 'R' &&
              runtime.bytes.at(0x9006u) == 0u &&
              runtime.trace.back() == "atexit",
          "negative index initializes one shared ERROR record");
    const auto call_count = runtime.trace.size();
    Check(Property::Assign(actor, 4, source, runtime) ==
              Property::kFallback &&
              runtime.trace.size() == call_count &&
              runtime.bytes.at(Property::kFallback) == 0xffu,
          "repeated invalid index reuses initialized fallback");

    runtime.bytes[0x9000u] = 0u;
    Property::Cleanup(runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0x9000u} &&
              runtime.words.at(Property::kFallback + 4u) == 0u &&
              runtime.words.at(Property::kFallback + 8u) == 0u &&
              runtime.words.at(Property::kFallback + 12u) == 0u &&
              runtime.bytes.at(Property::kGuard) == 0x81u,
          "cleanup frees owned fallback text and clears three words");

    Runtime failed;
    failed.allocation_succeeds = false;
    Check(Property::Assign(actor, 7, source, failed) ==
              Property::kFallback &&
              failed.words.at(Property::kFallback + 4u) == 0u &&
              failed.words.at(Property::kFallback + 8u) == 0u &&
              failed.trace.back() == "atexit" &&
              std::find(failed.trace.begin(), failed.trace.end(),
                        "literal") == failed.trace.end(),
          "fallback registers cleanup even after text allocation failure");
}

void CleanupSharedAndEmpty() {
    Runtime shared;
    shared.words[Property::kFallback + 4u] = 0x9001u;
    shared.words[Property::kFallback + 8u] = 5u;
    shared.bytes[0x9000u] = 3u;
    Property::Cleanup(shared);
    Check(shared.bytes.at(0x9000u) == 2u && shared.freed.empty() &&
              shared.words.at(Property::kFallback + 4u) == 0u,
          "shared fallback decrements reference count");
    Property::Cleanup(shared);
    Check(shared.freed.empty(), "repeated cleanup does not release again");

    Runtime sentinel;
    sentinel.words[Property::kFallback + 4u] = 0x9001u;
    sentinel.bytes[0x9000u] = 0xffu;
    Property::Cleanup(sentinel);
    Check(sentinel.freed == std::vector<std::uint32_t>{0x9000u},
          "255 sentinel releases backing allocation");
}

void ShareAndCopy() {
    Runtime share;
    share.words[source + 4u] = 0x7001u;
    share.words[source + 8u] = 3u;
    share.words[source + 12u] = 8u;
    share.bytes[0x7000u] = 1u;
    const auto destination = actor + 1112u + 3u * 16u;
    Check(Property::Assign(actor, 3, source, share) == destination &&
              share.words.at(destination + 4u) == 0x7001u &&
              share.words.at(destination + 8u) == 3u &&
              share.words.at(destination + 12u) == 8u &&
              share.bytes.at(0x7000u) == 2u &&
              share.trace.front() == "set_error" &&
              std::find(share.trace.begin(), share.trace.end(),
                        "allocate:1") == share.trace.end(),
          "non-self share reuses source buffer and increments header");

    Runtime copy;
    copy.words[source + 4u] = 0x7001u;
    copy.words[source + 8u] = 3u;
    copy.bytes[0x7000u] = 0xfeu;
    copy.bytes[0x7001u] = 'A';
    copy.bytes[0x7002u] = 'B';
    copy.bytes[0x7003u] = 'C';
    Check(Property::Assign(actor, 0, source, copy) == actor + 1112u &&
              copy.allocated_length == 3u && copy.allocated_mode == 1u &&
              copy.bytes.at(0x8001u) == 'A' &&
              copy.bytes.at(0x8003u) == 'C' &&
              copy.bytes.at(0x8004u) == 0u &&
              copy.words.at(actor + 1120u) == 3u,
          "saturated reference header causes a deep copy");

    Runtime empty;
    empty.words[source + 8u] = 0u;
    Check(Property::Assign(actor, 1, source, empty) == actor + 1128u &&
              empty.allocated_length == 0u &&
              empty.allocated_mode == 1u &&
              std::find(empty.trace.begin(), empty.trace.end(),
                        "set_error") == empty.trace.end(),
          "empty source enters copy allocation branch");

    Runtime failed;
    failed.allocation_succeeds = false;
    failed.words[source + 8u] = 2u;
    failed.bytes[Property::kEmptyText - 1u] = 0xffu;
    Check(Property::Assign(actor, 2, source, failed) == actor + 1144u &&
              failed.trace.back() == "allocate:1" &&
              failed.words[actor + 1152u] == 0u,
          "failed copy allocation returns destination without setting size");
}

void SelfAssignment() {
    const auto destination = actor + 1112u;
    Runtime stable;
    stable.words[destination + 4u] = 0x7001u;
    stable.words[destination + 8u] = 3u;
    Check(Property::Assign(actor, 0, destination, stable) == destination &&
              stable.trace == std::vector<std::string>{"detach", "detach"},
          "stable self-assignment calls detach twice without allocation");

    Runtime reset;
    reset.words[destination + 4u] = 0x7001u;
    reset.words[destination + 8u] = 3u;
    reset.detach_zeros_size = true;
    Check(Property::Assign(actor, 0, destination, reset) == destination &&
              reset.allocated_length == 3u &&
              reset.allocated_mode == 0u &&
              reset.trace == std::vector<std::string>{
                  "detach", "copy:0", "allocate:0", "size", "detach"},
          "self detach that clears size enters zero-byte copy and resize path");

    Runtime out_of_range;
    out_of_range.words[destination + 8u] = 3u;
    out_of_range.shrink_second_self_read = true;
    out_of_range.self_size_reads = 0u;
    bool caught = false;
    try {
        static_cast<void>(Property::Assign(actor, 0, destination,
                                           out_of_range));
    } catch (const std::out_of_range&) {
        caught = true;
    }
    Check(caught && out_of_range.trace.empty(),
          "self range check occurs before either detach");
}

}  // namespace

int main() {
    try {
        InvalidIndexAndCleanup();
        CleanupSharedAndEmpty();
        ShareAndCopy();
        SelfAssignment();
        std::cout << "actor_manager_player_text_property_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
