#include "pal4/global_object_registry.h"

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

struct Runtime final : pal4::IGlobalObjectRegistryRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    int registration_result = -17;

    [[nodiscard]] std::uint8_t ReadByte(
        const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
        events.emplace_back("byte:" + std::to_string(address));
    }
    [[nodiscard]] std::uint32_t ReadWord(
        const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("word:" + std::to_string(address));
    }
    [[nodiscard]] std::uint32_t Allocate(
        const std::uint32_t bytes) override {
        Check(bytes == 12u, "allocates twelve-byte list sentinel");
        events.emplace_back("allocate");
        return 0x1000u;
    }
    void Free(const std::uint32_t address) override {
        events.emplace_back("free:" + std::to_string(address));
    }
    [[nodiscard]] std::uint8_t InitialStackByte() override {
        events.emplace_back("stack-byte");
        return 0x5au;
    }
    [[nodiscard]] int RegisterAtExit(
        const std::uint32_t callback) override {
        Check(callback == 0x5fbfd0u,
              "registers the native cleanup entry");
        events.emplace_back("register");
        return registration_result;
    }
};

}  // namespace

int main() {
    using Registry = pal4::GlobalObjectRegistry;
    Runtime runtime;
    Check(Registry::InitializeAndRegister(runtime) == -17 &&
              runtime.bytes[Registry::kStackByte] == 0x5au &&
              runtime.words[0x1000u] == 0x1000u &&
              runtime.words[0x1004u] == 0x1000u &&
              runtime.words[Registry::kSentinel] == 0x1000u &&
              runtime.words[Registry::kCount] == 0u &&
              runtime.events == std::vector<std::string>({
                  "stack-byte", "byte:9631448", "allocate", "word:4096",
                  "word:4100", "word:9631452", "word:9631456",
                  "register"}),
          "initialize preserves stack-sourced byte and registers cleanup");
    runtime.events.clear();
    runtime.bytes[Registry::kGuard] = 0x80u;
    Registry::Cleanup(runtime);
    Check(runtime.bytes[Registry::kGuard] == 0x81u &&
              runtime.words[Registry::kSentinel] == 0u &&
              runtime.words[Registry::kCount] == 0u &&
              runtime.events[0] == "byte:9631444" &&
              runtime.events[1] == "free:4096",
          "empty cleanup sets guard, frees sentinel and clears globals");
    runtime.events.clear();
    Registry::Cleanup(runtime);
    Check(runtime.events.empty(), "guarded repeated cleanup is a no-op");

    Runtime populated;
    (void)Registry::Initialize(populated);
    populated.events.clear();
    populated.words[0x1000u] = 0x2000u;
    populated.words[0x1004u] = 0x3000u;
    populated.words[0x2000u] = 0x3000u;
    populated.words[0x2004u] = 0x1000u;
    populated.words[0x3000u] = 0x1000u;
    populated.words[0x3004u] = 0x2000u;
    populated.words[Registry::kCount] = 2u;
    Registry::Cleanup(populated);
    Check(populated.words[0x1000u] == 0x1000u &&
              populated.words[0x1004u] == 0x1000u &&
              populated.words[Registry::kSentinel] == 0u &&
              populated.words[Registry::kCount] == 0u,
          "cleanup unlinks both nodes and resets sentinel state");
    std::vector<std::string> frees;
    for (const auto& event : populated.events)
        if (event.starts_with("free:")) frees.push_back(event);
    Check(frees == std::vector<std::string>({
              "free:8192", "free:12288", "free:4096"}),
          "cleanup frees nodes in forward order before sentinel");
    pal4::GlobalObjectRegistryRuntimeStub stub;
    Check(Registry::InitializeAndRegister(stub) == 0,
          "allocator and CRT boundary stub links");
    return EXIT_SUCCESS;
}
