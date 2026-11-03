#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class INumericSpinnerPoolRuntime {
public:
    virtual ~INumericSpinnerPoolRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual int RegisterCleanup(
        std::uint32_t function_address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructSpinner(
        std::uint32_t memory, std::string_view name,
        std::int32_t value) = 0;
    virtual void UpdateValue(std::uint32_t spinner,
                             std::int32_t value) = 0;
    virtual void RemoveChild(std::uint32_t parent,
                             std::uint32_t child) = 0;
    virtual void DeleteSpinner(std::uint32_t spinner,
                               std::uint32_t flag) = 0;
};

class NumericSpinnerPool final {
public:
    static constexpr std::uint32_t kBegin = 0x8edf8cu;
    static constexpr std::uint32_t kEnd = 0x8edf90u;
    static constexpr std::uint32_t kCapacityEnd = 0x8edf94u;
    static constexpr std::uint32_t kSerial = 0x8edf98u;
    static constexpr std::uint32_t kCleanupGuard = 0x8edf84u;
    static constexpr std::uint32_t kInitialByte = 0x8edf88u;

    // IDA 0x4afbc0.
    [[nodiscard]] static std::uint32_t CreateOrGet(
        std::int32_t value, INumericSpinnerPoolRuntime& runtime);
    // IDA 0x4afcc0; return differs between spare and growth branches.
    [[nodiscard]] static std::uint32_t Remove(
        std::uint32_t spinner, INumericSpinnerPoolRuntime& runtime);
    // IDA 0x4afed0.
    [[nodiscard]] static std::uint32_t Clear(
        INumericSpinnerPoolRuntime& runtime);
    // IDA 0x4aff30.
    [[nodiscard]] static std::uint32_t Initialize(
        INumericSpinnerPoolRuntime& runtime);
    // IDA 0x4b1890, including its tail chunk at 0x4b18c0.
    [[nodiscard]] static int InitializeGlobalsAndRegisterCleanup(
        std::uint32_t seed, INumericSpinnerPoolRuntime& runtime);
    // IDA 0x4b18a0.
    [[nodiscard]] static int InitializeGlobals(
        std::uint32_t seed, INumericSpinnerPoolRuntime& runtime);
    // IDA 0x4b18d0.
    static void CleanupGlobals(INumericSpinnerPoolRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t spinner, INumericSpinnerPoolRuntime& runtime);
};

}  // namespace pal4
