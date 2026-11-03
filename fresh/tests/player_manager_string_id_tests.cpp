#include "pal4/player_manager_string_id.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IStringBufferRuntime {
public:
    static constexpr std::array<std::uint32_t, 4> kTexts{
        0x8a5900u, 0x8a58fcu, 0x8a58f8u, 0x8a58f4u};

    Runtime() {
        const std::array<const char*, 4> names{"101", "103", "106", "105"};
        for (std::size_t i = 0; i < names.size(); ++i) {
            Put(kTexts[i], names[i]);
            WriteDword(pal4::PlayerManagerStringId::kCandidatePointers[i],
                       kTexts[i]);
        }
    }

    void Put(std::uint32_t address, const std::string& value) {
        for (char character : value)
            bytes_[address++] = static_cast<std::uint8_t>(character);
        bytes_[address] = 0u;
    }

    std::uint32_t Allocate(std::uint32_t) override {
        if (++allocation_attempts == throw_on_allocation)
            throw std::runtime_error("allocation failure");
        const auto address = next_allocation;
        next_allocation += 0x20u;
        allocations.push_back(address);
        return address;
    }
    std::uint32_t Free(std::uint32_t address) override {
        frees.push_back(address);
        events.push_back("free:" + std::to_string(address));
        return 0u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        if (throw_during_compare && address >= 0x10000u &&
            address < 0x20000u)
            throw std::runtime_error("comparison failure");
        return bytes_[address];
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        candidate_pointer_reads.push_back(address);
        std::uint32_t value = 0u;
        for (std::uint32_t i = 0u; i < 4u; ++i)
            value |= static_cast<std::uint32_t>(bytes_[address + i]) << (i * 8u);
        return value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t size) override {
        copied_sources.push_back(source);
        events.push_back("copy:" + std::to_string(source));
        for (std::uint32_t i = 0u; i < size; ++i)
            bytes_[destination + i] = bytes_[source + i];
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        for (std::uint32_t i = 0u; i < 4u; ++i)
            bytes_[address + i] = static_cast<std::uint8_t>(value >> (i * 8u));
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes_[address] = value;
    }
    std::uint32_t FindSubstring(std::uint32_t, std::uint32_t) override { return 0u; }
    std::uint32_t FindCharacter(std::uint32_t, std::int32_t) override { return 0u; }
    std::uint32_t FindLastCharacter(std::uint32_t, std::int32_t) override { return 0u; }
    std::int32_t CompareCaseInsensitive(std::uint32_t, std::uint32_t) override { return 0; }
    void ConvertUppercase(std::uint32_t) override {}
    void ConvertLowercase(std::uint32_t) override {}

    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> copied_sources;
    std::vector<std::uint32_t> candidate_pointer_reads;
    std::vector<std::string> events;
    std::uint32_t allocation_attempts{};
    std::uint32_t throw_on_allocation{};
    bool throw_during_compare{};

private:
    std::map<std::uint32_t, std::uint8_t> bytes_;
    std::uint32_t next_allocation{0x10000u};
};

void CheckLookup(const char* text, std::uint32_t expected,
                 std::uint32_t comparisons) {
    Runtime runtime;
    runtime.Put(0x2000u, text);
    Check(pal4::PlayerManagerStringId::Lookup(0x2000u, runtime) == expected,
          "lookup result follows the four IDA candidate strings");
    Check(runtime.copied_sources.size() == comparisons + 1u &&
          runtime.copied_sources.front() == 0x2000u,
          "input is built once before the candidates");
    Check(runtime.candidate_pointer_reads.size() == comparisons,
          "only visited candidate pointers are read");
    for (std::uint32_t i = 0u; i < comparisons; ++i) {
        Check(runtime.candidate_pointer_reads[i] ==
                  pal4::PlayerManagerStringId::kCandidatePointers[i] &&
              runtime.copied_sources[i + 1u] == Runtime::kTexts[i],
              "candidates retain their IDA lookup order");
    }
    Check(runtime.allocations.size() == comparisons + 1u &&
          runtime.frees.size() == runtime.allocations.size() &&
          runtime.frees.back() == runtime.allocations.front(),
          "every temporary is released, with input last");
    for (std::uint32_t i = 0u; i < comparisons; ++i)
        Check(runtime.frees[i] == runtime.allocations[i + 1u],
              "candidate is released immediately after comparison");
}

void CheckExceptions() {
    Runtime runtime;
    runtime.Put(0x2000u, "105");
    runtime.throw_on_allocation = 3u;
    bool threw = false;
    try {
        static_cast<void>(pal4::PlayerManagerStringId::Lookup(0x2000u, runtime));
    } catch (const std::runtime_error&) { threw = true; }
    Check(threw && runtime.frees.size() == 2u &&
          runtime.frees[0] == runtime.allocations[1] &&
          runtime.frees[1] == runtime.allocations[0],
          "allocation exception releases the prior candidate and input");

    Runtime comparison_runtime;
    comparison_runtime.Put(0x2000u, "101");
    comparison_runtime.throw_during_compare = true;
    threw = false;
    try {
        static_cast<void>(pal4::PlayerManagerStringId::Lookup(
            0x2000u, comparison_runtime));
    } catch (const std::runtime_error&) { threw = true; }
    Check(threw && comparison_runtime.frees.size() == 2u &&
          comparison_runtime.frees[0] == comparison_runtime.allocations[1] &&
          comparison_runtime.frees[1] == comparison_runtime.allocations[0],
          "comparison exception releases the active candidate and input");
}

}  // namespace

int main() {
    CheckLookup("101", 0u, 1u);
    CheckLookup("103", 1u, 2u);
    CheckLookup("106", 2u, 3u);
    CheckLookup("105", 3u, 4u);
    CheckLookup("999", 4u, 4u);
    CheckLookup("", 4u, 4u);
    CheckLookup("101x", 4u, 4u);
    CheckExceptions();
}
