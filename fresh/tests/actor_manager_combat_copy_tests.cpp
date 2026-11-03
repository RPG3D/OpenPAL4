#include "pal4/actor_manager_combat_actor.h"

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

constexpr std::uint32_t object = 0x5000u;
constexpr std::uint32_t source = 0x9000u;
constexpr std::uint32_t source_records = source + 336u;
constexpr std::uint32_t destination_records = object + 896u;

struct Runtime final : pal4::IActorManagerCombatCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> allocations;
    std::vector<std::string> text_events;
    std::vector<std::string> raw_events;
    unsigned shares = 0u;
    unsigned allocations_attempted = 0u;
    unsigned sizes_set = 0u;
    unsigned erases = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void EraseText(std::uint32_t record, std::uint32_t start,
                   std::uint32_t count) override {
        text_events.emplace_back("erase:" + std::to_string(record) + ":" +
                                 std::to_string(start) + ":" +
                                 std::to_string(count));
        ++erases;
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t value) override {
        Check(value == 1u, "shared text sets flag one");
        text_events.emplace_back("share:" + std::to_string(record));
        ++shares;
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(mode == 1u, "text allocation mode one");
        text_events.emplace_back("allocate:" + std::to_string(record) + ":" +
                                 std::to_string(length));
        ++allocations_attempted;
        const auto allocation = allocations.find(record);
        if (allocation == allocations.end()) return false;
        words[record + 4u] = allocation->second;
        return true;
    }
    void SetTextSize(std::uint32_t record,
                     std::uint32_t length) override {
        words[record + 8u] = length;
        bytes[words[record + 4u] + length] = 0u;
        text_events.emplace_back("size:" + std::to_string(record));
        ++sizes_set;
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t from, std::uint32_t count) override {
        if (count == 336u) {
            raw_events.emplace_back("raw:" + std::to_string(destination) +
                                    ":" + std::to_string(from));
        } else {
            text_events.emplace_back("copy:" + std::to_string(destination) +
                                     ":" + std::to_string(count));
        }
        for (std::uint32_t index = 0u; index < count; ++index)
            bytes[destination + index] = bytes[from + index];
    }
};

void SetText(Runtime& runtime, std::uint32_t record,
             std::uint32_t data, std::uint32_t length,
             std::uint32_t capacity, std::uint8_t references) {
    runtime.words[record + 4u] = data;
    runtime.words[record + 8u] = length;
    runtime.words[record + 12u] = capacity;
    if (data) runtime.bytes[data - 1u] = references;
}

void MixedCopyPaths() {
    Runtime runtime;
    for (std::uint32_t word = 0u; word < 18u; ++word)
        runtime.words[source_records + word * 4u] = 0xa000u + word;
    for (std::uint32_t word = 0u; word < 3u; ++word)
        runtime.words[source_records + 248u + word * 4u] = 0xb000u + word;
    for (std::uint32_t offset = 0u; offset < 336u; ++offset)
        runtime.bytes[source + offset] = static_cast<std::uint8_t>(offset);

    SetText(runtime, source_records + 72u, 0x3000u, 3u, 5u, 2u);
    SetText(runtime, source_records + 88u, 0x3100u, 2u, 4u, 0xfeu);
    SetText(runtime, source_records + 104u, 0u, 1u, 6u, 0u);
    SetText(runtime, source_records + 168u, 0x3300u, 4u, 8u, 1u);
    runtime.bytes[0x841963u] = 7u;
    runtime.bytes[0x3100u] = 'A';
    runtime.bytes[0x3101u] = 'B';
    runtime.allocations[destination_records + 88u] = 0x4000u;
    runtime.words[destination_records + 124u] = 0xdeadu;

    Check(pal4::ActorManagerCombatActor::CopyFrom(
              object, source, runtime) == object,
          "copy returns the destination actor");
    for (std::uint32_t word = 0u; word < 18u; ++word)
        Check(runtime.words[destination_records + word * 4u] ==
                  0xa000u + word,
              "first 18 fields copied from source plus 336");
    for (std::uint32_t word = 0u; word < 3u; ++word)
        Check(runtime.words[destination_records + 248u + word * 4u] ==
                  0xb000u + word,
              "three tail fields copied after text records");
    Check(runtime.shares == 3u && runtime.allocations_attempted == 8u &&
              runtime.sizes_set == 1u && runtime.erases == 0u,
          "all 11 records use the expected share or allocation path");
    Check(runtime.words[destination_records + 76u] == 0x3000u &&
              runtime.words[destination_records + 80u] == 3u &&
              runtime.words[destination_records + 84u] == 5u &&
              runtime.bytes[0x2fffu] == 3u,
          "share path copies buffer state and increments reference");
    Check(runtime.words[destination_records + 92u] == 0x4000u &&
              runtime.words[destination_records + 96u] == 2u &&
              runtime.bytes[0x4000u] == 'A' &&
              runtime.bytes[0x4001u] == 'B' &&
              runtime.bytes[0x4002u] == 0u &&
              runtime.bytes[0x30ffu] == 0xfeu,
          "saturated reference uses deep copy and terminator");
    Check(runtime.words[destination_records + 108u] == 0x841964u &&
              runtime.bytes[0x841963u] == 8u &&
              runtime.words[destination_records + 124u] == 0xdeadu &&
              runtime.words[destination_records + 172u] == 0x3300u &&
              runtime.bytes[0x32ffu] == 2u,
          "empty fallback, failed allocation and second text group");
    Check(runtime.raw_events == std::vector<std::string>{
              "raw:20484:36864", "raw:20820:36864"},
          "two 336-byte raw copies follow the text fields");
    for (std::uint32_t index = 0u; index < 336u; ++index)
        Check(runtime.bytes[object + 4u + index] ==
                  static_cast<std::uint8_t>(index) &&
                  runtime.bytes[object + 340u + index] ==
                      static_cast<std::uint8_t>(index),
              "both raw regions match the source prefix");
}

void AliasedTextRecords() {
    Runtime runtime;
    const auto overlapping_source = object + 560u;
    for (std::uint32_t index = 0u; index < 6u; ++index)
        runtime.words[destination_records + 72u + index * 16u + 8u] =
            index + 1u;
    for (std::uint32_t index = 0u; index < 5u; ++index)
        runtime.words[destination_records + 168u + index * 16u + 8u] =
            index + 7u;
    Check(pal4::ActorManagerCombatActor::CopyFrom(
              object, overlapping_source, runtime) == object &&
              runtime.erases == 22u && runtime.shares == 0u &&
              runtime.allocations_attempted == 0u,
          "aliased 11 records call both erase operations");
    Check(runtime.text_events.front() ==
              "erase:" + std::to_string(destination_records + 72u) +
                  ":1:4294967295" &&
              runtime.text_events.back() ==
                  "erase:" + std::to_string(
                      destination_records + 168u + 4u * 16u) + ":0:0",
          "aliased record order spans six then five text fields");
}

}  // namespace

int main() {
    try {
        MixedCopyPaths();
        AliasedTextRecords();
        std::cout << "actor_manager_combat_copy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
