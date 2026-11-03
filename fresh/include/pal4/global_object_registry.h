#pragma once

#include <cstdint>

namespace pal4 {

class IGlobalObjectRegistryRuntime {
public:
    virtual ~IGlobalObjectRegistryRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t InitialStackByte() = 0;
    [[nodiscard]] virtual int RegisterAtExit(
        std::uint32_t callback) = 0;
};

class GlobalObjectRegistryRuntimeStub final
    : public IGlobalObjectRegistryRuntime {
public:
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override {
        return 0u;
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override {
        return 0u;
    }
    void Free(std::uint32_t) override {}
    [[nodiscard]] std::uint8_t InitialStackByte() override { return 0u; }
    [[nodiscard]] int RegisterAtExit(std::uint32_t) override { return 0; }
};

class GlobalObjectRegistry final {
public:
    static constexpr std::uint32_t kGuard = 0x92f6d4u;
    static constexpr std::uint32_t kStackByte = 0x92f6d8u;
    static constexpr std::uint32_t kSentinel = 0x92f6dcu;
    static constexpr std::uint32_t kCount = 0x92f6e0u;

    // IDA 0x5fbf80 and its tail at 0x5fbfc0.
    [[nodiscard]] static int InitializeAndRegister(
        IGlobalObjectRegistryRuntime& runtime);
    // IDA 0x5fbf90.
    [[nodiscard]] static std::uint32_t Initialize(
        IGlobalObjectRegistryRuntime& runtime);
    // IDA 0x5fbfd0.
    static void Cleanup(IGlobalObjectRegistryRuntime& runtime);
};

}  // namespace pal4
