#include "pal4/global_memory_pool.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IGlobalMemoryPoolRuntime {
public:
    [[nodiscard]] std::uint32_t Read(
        const std::uint32_t address) const override {
        const auto found = words.find(address);
        return found == words.end() ? 0u : found->second;
    }

    void Write(const std::uint32_t address,
               const std::uint32_t value) override {
        words[address] = value;
    }

    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        const std::uint32_t aligned = address & ~3u;
        const unsigned shift = (address & 3u) * 8u;
        words[aligned] = (Read(aligned) & ~(0xffu << shift)) |
                         (static_cast<std::uint32_t>(value) << shift);
    }

    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }

    [[nodiscard]] std::uint32_t RegisterExitHandler(
        const std::uint32_t address) override {
        registered = address;
        recorded_size_at_registration = Read(pal4::GlobalMemoryPool::kAddress);
        return registration_result;
    }

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> freed;
    std::uint32_t registered = 0u;
    std::uint32_t recorded_size_at_registration = 0u;
    std::uint32_t registration_result = 7u;
};

void CheckInitializationLayout() {
    Runtime runtime;
    constexpr std::uint32_t base = 0x5000u;
    runtime.words[base + 28u] = 0xaabbccffu;
    runtime.words[base + 48u] = 0x1234u;
    Check(pal4::MemoryPoolInitializer::Initialize(
              base, 123u, 0x2000u, 16u, runtime) == base,
          "pool initializer returns its own address");
    Check(runtime.Read(base) == 0x2000u && runtime.Read(base + 4u) == 16u &&
              runtime.Read(base + 8u) == 15u &&
              runtime.Read(base + 12u) == 16u &&
              runtime.Read(base + 16u) == 0x2000u,
          "size and normalized alignment fields match IDA calculation");
    Check(runtime.Read(base + 20u) == 0u &&
              runtime.Read(base + 24u) == 0u &&
              runtime.Read(base + 28u) == 0xaabbcc00u &&
              runtime.Read(base + 32u) == 0u &&
              runtime.Read(base + 36u) == 0u &&
              runtime.Read(base + 40u) == 0u &&
              runtime.Read(base + 44u) == 0u &&
              runtime.Read(base + 48u) == 0x1234u,
          "observed fields clear while upper bytes and later fields remain");
}

void CheckSizeClampAndAlignmentRounding() {
    Runtime runtime;
    constexpr std::uint32_t base = 0x6000u;
    (void)pal4::MemoryPoolInitializer::Initialize(base, 0u, 1u, 19u, runtime);
    Check(runtime.Read(base) == 0x1000u &&
              runtime.Read(base + 16u) == 0x1000u &&
              runtime.Read(base + 28u) == 1u &&
              runtime.Read(base + 4u) == 16u,
          "small size is clamped but byte field retains original low byte");
    (void)pal4::MemoryPoolInitializer::Initialize(base, 0u, 0x1000u, 7u,
                                                   runtime);
    Check(runtime.Read(base + 4u) == 4u &&
              runtime.Read(base + 8u) == 3u &&
              runtime.Read(base + 12u) == 8u,
          "alignment is rounded down to multiple of four then power of two");
    (void)pal4::MemoryPoolInitializer::Initialize(base, 0u, 0x1000u, 31u,
                                                   runtime);
    Check(runtime.Read(base + 4u) == 32u &&
              runtime.Read(base + 8u) == 31u &&
              runtime.Read(base + 12u) == 32u,
          "normalized alignment grows to next power of two");
}

void CheckGlobalStartupAndRegistration() {
    Runtime runtime;
    Check(pal4::GlobalMemoryPool::InitializeAndRegisterExit(runtime) == 7u,
          "startup returns CRT exit registration result");
    Check(runtime.registered == pal4::GlobalMemoryPool::kCleanupEntry &&
              runtime.recorded_size_at_registration == 0x2000u &&
              runtime.Read(pal4::GlobalMemoryPool::kAddress + 4u) == 16u,
          "global pool is initialized before cleanup handler registration");
}

void CheckCleanupChainsAndBuffer() {
    Runtime runtime;
    constexpr std::uint32_t base = pal4::GlobalMemoryPool::kAddress;
    runtime.words[base + 24u] = 0x1000u;
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x2000u] = 0u;
    runtime.words[base + 20u] = 0x3000u;
    runtime.words[0x3000u] = 0u;
    runtime.words[base + 32u] = 0x4000u;
    runtime.words[base + 36u] = 99u;
    runtime.words[base + 40u] = 77u;
    pal4::GlobalMemoryPool::Cleanup(runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{
              0x1000u, 0x2000u, 0x3000u, 0x4000u},
          "first chain, second chain and pool buffer free in IDA order");
    Check(runtime.Read(base + 24u) == 0u &&
              runtime.Read(base + 20u) == 0u &&
              runtime.Read(base + 32u) == 0u &&
              runtime.Read(base + 36u) == 0u &&
              runtime.Read(base + 40u) == 0u,
          "all observed head and buffer state clears");
}

void CheckCleanupEmptyStillFreesBufferSlot() {
    Runtime runtime;
    pal4::GlobalMemoryPool::Cleanup(runtime);
    Check(runtime.freed == std::vector<std::uint32_t>{0u},
          "empty cleanup still calls MemFree for null buffer slot");
}

}  // namespace

int main() {
    CheckInitializationLayout();
    CheckSizeClampAndAlignmentRounding();
    CheckGlobalStartupAndRegistration();
    CheckCleanupChainsAndBuffer();
    CheckCleanupEmptyStillFreesBufferSlot();
    std::cout << "5 global memory pool test cases passed\n";
}
