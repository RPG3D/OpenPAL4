#pragma once

#include <cstdint>

namespace pal4 {

struct StaticCharacterBufferState {
    std::uint8_t wide_header_byte = 0u;
    std::uint8_t multibyte_header_byte = 0u;
    std::uint8_t cleanup_bits = 0u;
    std::uint32_t wide_begin = 0u;
    std::uint32_t wide_end = 0u;
    std::uint32_t wide_capacity_end = 0u;
    std::uint32_t multibyte_begin = 0u;
    std::uint32_t multibyte_end = 0u;
    std::uint32_t multibyte_capacity_end = 0u;
};

class IStaticCharacterBufferRuntime {
public:
    virtual ~IStaticCharacterBufferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint16_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterExit(
        std::uint32_t callback_address) = 0;
};

class StaticCharacterBuffers final {
public:
    static constexpr std::uint32_t kWideHeaderAddress = 0x8e40b4u;
    static constexpr std::uint32_t kWideBeginAddress = 0x8e40b8u;
    static constexpr std::uint32_t kWideEndAddress = 0x8e40bcu;
    static constexpr std::uint32_t kWideCapacityAddress = 0x8e40c0u;
    static constexpr std::uint32_t kMultibyteHeaderAddress = 0x8e40a0u;
    static constexpr std::uint32_t kMultibyteBeginAddress = 0x8e40a4u;
    static constexpr std::uint32_t kMultibyteEndAddress = 0x8e40a8u;
    static constexpr std::uint32_t kMultibyteCapacityAddress = 0x8e40acu;
    static constexpr std::uint32_t kCleanupBitsAddress = 0x8e40b0u;
    static constexpr std::uint32_t kWideCleanupEntry = 0x410360u;
    static constexpr std::uint32_t kMultibyteCleanupEntry = 0x410410u;

    explicit StaticCharacterBuffers(StaticCharacterBufferState& state)
        : state_(state) {}

    [[nodiscard]] std::uint32_t InitializeWide(
        std::uint8_t incoming_stack_byte,
        IStaticCharacterBufferRuntime& runtime);
    [[nodiscard]] int InitializeWideAndRegister(
        std::uint8_t incoming_stack_byte,
        IStaticCharacterBufferRuntime& runtime);
    void CleanupWide(IStaticCharacterBufferRuntime& runtime);

    [[nodiscard]] std::uint32_t InitializeMultibyte(
        std::uint8_t incoming_stack_byte,
        IStaticCharacterBufferRuntime& runtime);
    [[nodiscard]] int InitializeMultibyteAndRegister(
        std::uint8_t incoming_stack_byte,
        IStaticCharacterBufferRuntime& runtime);
    void CleanupMultibyte(IStaticCharacterBufferRuntime& runtime);

private:
    StaticCharacterBufferState& state_;
};

}  // namespace pal4
