#include "pal4/combat_log_buffer_lifecycle.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatLogBufferLifecycleRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read-byte:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::int32_t Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        return 77;
    }
    std::int32_t DestroyFacet(std::uint32_t facet,
                              std::uint32_t flags) override {
        calls.push_back("destroy-facet:" + std::to_string(facet) +
                        ":" + std::to_string(flags));
        return 88;
    }
};

std::size_t FindCall(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0u; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

void PrepareBuffer(Runtime& runtime, std::uint32_t buffer,
                   std::uint32_t flags, std::uint32_t facet = 0u) {
    runtime.bytes[buffer + 68u] = static_cast<std::uint8_t>(flags);
    runtime.words[buffer + 68u] = flags;
    runtime.words[buffer + 60u] = 0x5000u;
    runtime.words[buffer + 52u] = facet;
    runtime.words[buffer + 12u] = 0x3000u;
    runtime.words[0x3000u] = 0x4000u;
}

void DirectBufferCleanup() {
    Runtime plain;
    PrepareBuffer(plain, 0x1000u, 0x100u);
    Check(pal4::CombatLogBufferLifecycle::CleanupBuffer(
              0x1000u, plain) == 0 &&
              plain.words[0x1000u] == 0x868310u &&
              plain.words[0x1000u + 60u] == 0u &&
              plain.words[0x1000u + 68u] == 0x100u,
          "unowned buffer clears position and keeps other flags");
    Check(FindCall(plain, "write:4096:8683852") <
              FindCall(plain, "write:4096:8815376"),
          "derived vtable precedes base streambuf vtable");

    Runtime owned;
    PrepareBuffer(owned, 0x1000u, 0x105u);
    Check(pal4::CombatLogBufferLifecycle::CleanupBuffer(
              0x1000u, owned) == 77 &&
              owned.words[0x1000u + 68u] == 0x104u,
          "owned storage free result and ownership-bit removal");
    Check(FindCall(owned, "write:4096:8683852") <
              FindCall(owned, "free:16384") &&
              FindCall(owned, "free:16384") <
              FindCall(owned, "write:4096:8815376"),
          "storage released between derived and base vtable writes");
}

void FacetReferenceCounts() {
    for (const auto [count, expected_count, expected_result, destroyed] : {
             std::tuple{0u, 0u, 88, true},
             std::tuple{1u, 0u, 88, true},
             std::tuple{2u, 1u, 1, false},
             std::tuple{0xffffffffu, 0xffffffffu, -1, false}}) {
        Runtime runtime;
        PrepareBuffer(runtime, 0x1000u, 0u, 0x6000u);
        runtime.words[0x6000u + 4u] = count;
        Check(pal4::CombatLogBufferLifecycle::CleanupBuffer(
                  0x1000u, runtime) == expected_result &&
                  runtime.words[0x6000u + 4u] == expected_count,
              "facet count transition and return value");
        const bool called = [&] {
            for (const auto& call : runtime.calls)
                if (call == "destroy-facet:24576:1") return true;
            return false;
        }();
        Check(called == destroyed,
              "facet is destroyed only when reference count reaches zero");
    }
}

void OuterStreamCleanup() {
    Runtime runtime;
    constexpr std::uint32_t kIos = 0x2000u;
    constexpr std::uint32_t kStream = kIos - 80u;
    constexpr std::uint32_t kBuffer = kIos - 76u;
    runtime.words[kStream] = 0x848180u;
    runtime.words[0x848184u] = 80u;
    PrepareBuffer(runtime, kBuffer, 1u);
    Check(pal4::CombatLogBufferLifecycle::CleanupStream(
              kIos, runtime) == 0x848180u &&
              runtime.words[kIos] == 0x8682c8u &&
              runtime.words[kBuffer] == 0x868310u &&
              runtime.words[kBuffer + 60u] == 0u,
          "outer cleanup returns original stream vtable");
    Check(FindCall(runtime, "write:8192:8683848") <
              FindCall(runtime, "write:8116:8683852") &&
              FindCall(runtime, "write:8116:8683852") <
              FindCall(runtime, "free:16384") &&
              FindCall(runtime, "free:16384") <
              FindCall(runtime, "write:8116:8815376") &&
              FindCall(runtime, "write:8192:8815304") ==
                  runtime.calls.size() - 1u,
          "outer and embedded vtable writes follow IDA order");
}

}  // namespace

int main() {
    try {
        DirectBufferCleanup();
        FacetReferenceCounts();
        OuterStreamCleanup();
        std::cout << "combat log buffer lifecycle tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
