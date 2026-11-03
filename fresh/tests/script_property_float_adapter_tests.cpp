#include "pal4/script_property_float_adapter.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kLink = 0x2000u;

struct Write {
    std::uint32_t address, value, width;
    bool operator==(const Write&) const = default;
};

struct Runtime final : pal4::IScriptPropertyFloatRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Write> writes;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> forwarded_arguments;
    std::uint32_t forwarded_link = 0xffffffffu;
    std::uint32_t forwarded_offset = 0u;
    pal4::ScriptForwardResult returned{0x55u, 12.5, true};

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kObject + 8u, "link address");
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.push_back({address, value, 4u});
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        writes.push_back({address, value, 1u});
    }
    pal4::ScriptForwardResult PreservedRegisters() override {
        throw std::runtime_error("unconditional property call preserved registers");
    }
    pal4::ScriptForwardResult CallVirtual(
        std::uint32_t link, std::uint32_t offset,
        std::span<const std::uint32_t> arguments) override {
        calls.emplace_back("virtual");
        forwarded_link = link;
        forwarded_offset = offset;
        forwarded_arguments.assign(arguments.begin(), arguments.end());
        return returned;
    }
};

void ConstructionAndFlags() {
    Runtime r;
    Check(pal4::ScriptPropertyFloatAdapter::Construct(kObject, r) == kObject,
          "constructor returns this");
    Check(r.writes == std::vector<Write>{{kObject + 4u, 0u, 1u},
                                         {kObject + 8u, 0u, 4u},
                                         {kObject + 12u, 0u, 1u},
                                         {kObject, 0x847f70u, 4u}},
          "constructor field and vtable write order");
    r.writes.clear();
    pal4::ScriptPropertyFloatAdapter::SetFlag(kObject, r);
    pal4::ScriptPropertyFloatAdapter::ClearFlag(kObject, r);
    Check(r.writes == std::vector<Write>{{kObject + 12u, 1u, 1u},
                                         {kObject + 12u, 0u, 1u}},
          "two flag entries only write byte 12");
}

void VirtualReads() {
    using Adjusted = pal4::ScriptForwardResult (*)(
        std::uint32_t, std::uint32_t, std::uint32_t,
        pal4::IScriptPropertyFloatRuntime&);
    constexpr std::array<std::pair<Adjusted, float>, 3> cases{{
        {pal4::ScriptPropertyFloatAdapter::ReadPlus005, 0.05f},
        {pal4::ScriptPropertyFloatAdapter::ReadPlus010, 0.1f},
        {pal4::ScriptPropertyFloatAdapter::ReadPlus020, 0.2f},
    }};
    for (const auto& [entry, adjustment] : cases) {
        Runtime r;
        r.words[kObject + 8u] = kLink;
        const auto adjusted = entry(kObject, 0x11u, 0u, r);
        Check(r.calls == std::vector<std::string>{"read", "virtual"} &&
                  r.forwarded_link == kLink && r.forwarded_offset == 0xc8u &&
                  r.forwarded_arguments == std::vector<std::uint32_t>{0x11u, 0u} &&
                  adjusted.has_st0 && adjusted.st0 == 12.5 + static_cast<double>(adjustment),
              "zero second argument adds the native float constant");

        r = Runtime{};
        r.words[kObject + 8u] = kLink;
        const auto untouched = entry(kObject, 0x22u, 0x33u, r);
        Check(r.forwarded_arguments == std::vector<std::uint32_t>{0x22u, 0x33u} &&
                  r.forwarded_offset == 0xc8u && untouched.st0 == 12.5 &&
                  untouched.eax == r.returned.eax,
              "nonzero second argument tail forwards without adjustment");
    }

    Runtime r;
    r.words[kObject + 8u] = 0u;
    const std::array<std::uint32_t, 3> args{1u, 2u, 3u};
    const auto raw = pal4::ScriptPropertyFloatAdapter::ForwardAtC8(
        kObject, args, r);
    Check(r.forwarded_link == 0u && r.forwarded_offset == 0xc8u &&
              r.forwarded_arguments == std::vector<std::uint32_t>{1u, 2u, 3u} &&
              raw.st0 == 12.5 && r.calls == std::vector<std::string>{"read", "virtual"},
          "plain tail forwards all stack words without a null check");
}

}  // namespace

int main() {
    try {
        ConstructionAndFlags();
        VirtualReads();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
