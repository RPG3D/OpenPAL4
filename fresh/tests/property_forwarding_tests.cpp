#include "pal4/property_forwarding.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPropertyForwardingRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t returned{0xdefau};

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t Invoke0(const std::uint32_t object,
                          const std::uint32_t slot) override {
        calls.push_back("call0:" + std::to_string(object) + ":" +
                        std::to_string(slot));
        return returned;
    }
    std::uint32_t Invoke1(const std::uint32_t object,
                          const std::uint32_t slot,
                          const std::uint32_t first) override {
        calls.push_back("call1:" + std::to_string(object) + ":" +
                        std::to_string(slot) + ":" +
                        std::to_string(first));
        return returned;
    }
    std::uint32_t Invoke2(const std::uint32_t object,
                          const std::uint32_t slot,
                          const std::uint32_t first,
                          const std::uint32_t second) override {
        calls.push_back("call2:" + std::to_string(object) + ":" +
                        std::to_string(slot) + ":" +
                        std::to_string(first) + ":" +
                        std::to_string(second));
        return returned;
    }
};

void TestDirectForwarding() {
    using Method = std::uint32_t (*)(
        std::uint32_t, std::uint32_t, pal4::IPropertyForwardingRuntime&);
    const std::array<std::pair<Method, std::uint32_t>, 4> methods{{
        {&pal4::PropertyForwarding::GetValue, 4u},
        {&pal4::PropertyForwarding::SetValue, 28u},
        {&pal4::PropertyForwarding::GetValue2, 8u},
        {&pal4::PropertyForwarding::SetValue2, 32u}}};
    for (const auto& [method, slot] : methods) {
        Runtime runtime;
        Check(method(0x3000u, 0x5000u, runtime) == runtime.returned &&
                  runtime.calls == std::vector<std::string>{
                      "call1:20480:" + std::to_string(slot) + ":12288"},
              "direct forwarding uses the selected property slot and context");
    }
}

void TestOffsetForwarding() {
    Runtime runtime;
    runtime.words[0x400cu] = 0x5000u;
    runtime.words[0x4010u] = 0x20u;
    runtime.words[0x4004u] = 0x6000u;
    runtime.words[0x4008u] = 0x40u;
    Check(pal4::PropertyForwarding::GetFromOffset(
              0x4000u, 3u, runtime) == runtime.returned &&
              runtime.calls == std::vector<std::string>{
                  "read:16396", "read:16400", "call1:20480:16:35"},
          "outer getter adds offset to base and invokes slot sixteen");
    runtime.calls.clear();
    Check(pal4::PropertyForwarding::SetFromOffset(
              0x4000u, 5u, 0xabcu, runtime) == runtime.returned &&
              runtime.calls == std::vector<std::string>{
                  "read:16396", "read:16400", "call2:20480:20:37:2748"},
          "outer setter forwards value after adjusted offset");
    runtime.calls.clear();
    Check(pal4::PropertyForwarding::GetFromVtable(
              0x4000u, runtime) == runtime.returned &&
              runtime.calls == std::vector<std::string>{
                  "read:16396", "call0:20480:4"},
          "zero-argument getter calls owner slot four");
    runtime.calls.clear();
    Check(pal4::PropertyForwarding::GetFromOffset2(
              0x4000u, 7u, runtime) == runtime.returned &&
              runtime.calls == std::vector<std::string>{
                  "read:16388", "read:16392", "call1:24576:16:71"},
          "inner getter uses owner and base at four and eight");
    runtime.calls.clear();
    Check(pal4::PropertyForwarding::SetFromOffset2(
              0x4000u, 9u, 0xdefu, runtime) == runtime.returned &&
              runtime.calls == std::vector<std::string>{
                  "read:16388", "read:16392", "call2:24576:20:73:3567"},
          "inner setter forwards second argument and result");

    pal4::PropertyForwardingRuntimeStub stub;
    Check(stub.ReadWord(0u) == 0u && stub.Invoke0(0u, 4u) == 0u &&
              stub.Invoke1(0u, 4u, 1u) == 0u &&
              stub.Invoke2(0u, 4u, 1u, 2u) == 0u,
          "native forwarding boundary has a linkable stub");
}

}  // namespace

int main() {
    TestDirectForwarding();
    TestOffsetForwarding();
}
