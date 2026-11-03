#include "pal4/script_property_forwarders.h"

#include <array>
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

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kLink = 0x2000u;

struct Runtime final : pal4::IScriptPropertyForwardRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> forwarded_arguments;
    std::uint32_t forwarded_offset = 0u;
    std::uint32_t expected_link = kLink;
    pal4::ScriptForwardResult incoming{0xdeadbeefu, 1.25, true};
    pal4::ScriptForwardResult returned{0x12345678u, 3.75, true};

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kObject + 8u, "forwarder link address");
        calls.emplace_back("read_link");
        return words.at(address);
    }
    pal4::ScriptForwardResult PreservedRegisters() override {
        calls.emplace_back("preserve");
        return incoming;
    }
    pal4::ScriptForwardResult CallVirtual(
        std::uint32_t link, std::uint32_t offset,
        std::span<const std::uint32_t> arguments) override {
        Check(link == expected_link, "forwarder child pointer");
        forwarded_offset = offset;
        forwarded_arguments.assign(arguments.begin(), arguments.end());
        calls.emplace_back("virtual");
        return returned;
    }
};

using Forwarder = pal4::ScriptForwardResult (*)(
    std::uint32_t, pal4::ScriptPropertyForwarders::Arguments,
    pal4::IScriptPropertyForwardRuntime&);

struct Case {
    std::uint32_t address, offset, argument_count;
    Forwarder forward;
};

constexpr Case kCases[] = {
    {0x56e530u, 28u, 1u, pal4::ScriptPropertyForwarders::Forward28},
    {0x56e540u, 32u, 1u, pal4::ScriptPropertyForwarders::Forward32},
    {0x56e550u, 36u, 2u, pal4::ScriptPropertyForwarders::Forward36},
    {0x56e560u, 40u, 2u, pal4::ScriptPropertyForwarders::Forward40},
    {0x56e570u, 44u, 2u, pal4::ScriptPropertyForwarders::Forward44},
    {0x56e580u, 48u, 2u, pal4::ScriptPropertyForwarders::Forward48},
    {0x56e590u, 52u, 2u, pal4::ScriptPropertyForwarders::Forward52},
    {0x56e5a0u, 56u, 2u, pal4::ScriptPropertyForwarders::Forward56},
    {0x56e5b0u, 60u, 2u, pal4::ScriptPropertyForwarders::Forward60},
    {0x56e5c0u, 64u, 2u, pal4::ScriptPropertyForwarders::Forward64},
    {0x56e5d0u, 68u, 3u, pal4::ScriptPropertyForwarders::Forward68},
    {0x56e5e0u, 72u, 3u, pal4::ScriptPropertyForwarders::Forward72},
    {0x56e5f0u, 76u, 3u, pal4::ScriptPropertyForwarders::Forward76},
    {0x56e600u, 80u, 1u, pal4::ScriptPropertyForwarders::Forward80},
    {0x56e610u, 84u, 1u, pal4::ScriptPropertyForwarders::Forward84},
    {0x56e620u, 88u, 1u, pal4::ScriptPropertyForwarders::Forward88},
    {0x56e630u, 92u, 1u, pal4::ScriptPropertyForwarders::Forward92},
    {0x56e640u, 96u, 4u, pal4::ScriptPropertyForwarders::Forward96},
    {0x56e650u, 100u, 4u, pal4::ScriptPropertyForwarders::Forward100},
    {0x56e660u, 104u, 4u, pal4::ScriptPropertyForwarders::Forward104},
    {0x56e670u, 108u, 4u, pal4::ScriptPropertyForwarders::Forward108},
};

void EveryForwarder() {
    const std::array<std::uint32_t, 4> arguments{
        0x11u, 0x22u, 0x33u, 0x44u};
    for (const auto& item : kCases) {
        Runtime r;
        const auto args = std::span<const std::uint32_t>(
            arguments.data(), item.argument_count);
        r.words[kObject + 8u] = 0u;
        const auto preserved = item.forward(kObject, args, r);
        Check(preserved.eax == 0xdeadbeefu &&
                  preserved.st0 == 1.25 && preserved.has_st0 &&
                  r.calls == std::vector<std::string>{
                      "read_link", "preserve"},
              "null link preserves native result registers");

        r = Runtime{};
        r.words[kObject + 8u] = kLink;
        const auto returned = item.forward(kObject, args, r);
        Check(returned.eax == 0x12345678u &&
                  returned.st0 == 3.75 && returned.has_st0 &&
                  r.forwarded_offset == item.offset &&
                  r.forwarded_arguments ==
                      std::vector<std::uint32_t>(args.begin(), args.end()) &&
                  r.calls == std::vector<std::string>{
                      "read_link", "virtual"},
              "link call forwards virtual slot, stack words and result");
        if (item.argument_count < arguments.size()) {
            r.calls.clear();
            bool rejected = false;
            try {
                (void)item.forward(kObject,
                                   std::span<const std::uint32_t>(arguments),
                                   r);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            Check(rejected && r.calls.empty(),
                  "C++ entry rejects mismatched stack-word count");
        }
    }
}

void UnguardedTailForwarders() {
    struct TailCase {
        std::uint32_t address, offset;
    };
    constexpr std::array<TailCase, 57> cases{{
        {0x56e680u, 0x70u}, {0x56e690u, 0x74u}, {0x56e6a0u, 0x78u},
        {0x56e6b0u, 0x7cu}, {0x56e6c0u, 0x94u}, {0x56e6d0u, 0xb4u},
        {0x56e6e0u, 0xa4u}, {0x56e6f0u, 0x90u}, {0x56e700u, 0xb0u},
        {0x56e720u, 0xa0u}, {0x56e740u, 0x80u}, {0x56e750u, 0x88u},
        {0x56e760u, 0x84u}, {0x56e770u, 0xbcu}, {0x56e780u, 0xc4u},
        {0x56e790u, 0xc0u}, {0x56e7a0u, 0xd0u}, {0x56e7c0u, 0xccu},
        {0x56e7e0u, 0xd4u}, {0x56e7f0u, 0xdcu}, {0x56e810u, 0xd8u},
        {0x56e830u, 0x98u}, {0x56e840u, 0xb8u}, {0x56e850u, 0xa8u},
        {0x56e860u, 0x8cu}, {0x56e870u, 0xacu}, {0x56e890u, 0x9cu},
        {0x56e8b0u, 0xe0u}, {0x56e8c0u, 0xe8u}, {0x56e8e0u, 0xe4u},
        {0x56e900u, 0xecu}, {0x56e910u, 0xf0u}, {0x56e920u, 0xf4u},
        {0x56e930u, 0xf8u}, {0x56e940u, 0xfcu}, {0x56e950u, 0x100u},
        {0x56e960u, 0x104u}, {0x56e970u, 0x108u}, {0x56e980u, 0x10cu},
        {0x56e990u, 0x110u}, {0x56e9a0u, 0x114u}, {0x56e9b0u, 0x118u},
        {0x56e9c0u, 0x11cu}, {0x56e9d0u, 0x120u}, {0x56e9e0u, 0x124u},
        {0x56e9f0u, 0x128u}, {0x56ea00u, 0x12cu}, {0x56ea10u, 0x130u},
        {0x56ea20u, 0x13cu}, {0x56ea30u, 0x148u}, {0x56ea50u, 0x154u},
        {0x56ea70u, 0x138u}, {0x56ea80u, 0x144u}, {0x56eaa0u, 0x150u},
        {0x56eac0u, 0x134u}, {0x56ead0u, 0x140u}, {0x56eaf0u, 0x14cu},
    }};
    constexpr std::array<std::uint32_t, 4> args{{0x11u, 0x22u, 0x33u, 0x44u}};
    for (const auto& item : cases) {
        Runtime r;
        r.words[kObject + 8u] = kLink;
        const auto value = pal4::ScriptPropertyForwarders::ForwardUnguarded(
            item.address, kObject, args, r);
        Check(r.forwarded_offset == item.offset &&
                  r.forwarded_arguments == std::vector<std::uint32_t>(args.begin(), args.end()) &&
                  r.calls == std::vector<std::string>{"read_link", "virtual"} &&
                  value.eax == r.returned.eax && value.has_st0 && value.st0 == r.returned.st0,
              "unguarded tail slot and stack forwarding");

        r = Runtime{};
        r.expected_link = 0u;
        r.words[kObject + 8u] = 0u;
        (void)pal4::ScriptPropertyForwarders::ForwardUnguarded(
            item.address, kObject, {}, r);
        Check(r.forwarded_offset == item.offset &&
                  r.calls == std::vector<std::string>{"read_link", "virtual"} &&
                  r.forwarded_arguments.empty(),
              "unguarded tail does not test link for null");
    }

    Runtime r;
    bool rejected = false;
    try {
        (void)pal4::ScriptPropertyForwarders::ForwardUnguarded(
            0x56e710u, kObject, {}, r);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Check(rejected && r.calls.empty(), "unknown tail entry rejected before memory access");
}

}  // namespace

int main() {
    try {
        EveryForwarder();
        UnguardedTailForwarders();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
