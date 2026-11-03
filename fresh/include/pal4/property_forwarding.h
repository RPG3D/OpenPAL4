#pragma once

#include <cstdint>

namespace pal4 {

class IPropertyForwardingRuntime {
public:
    virtual ~IPropertyForwardingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Invoke0(
        std::uint32_t object, std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::uint32_t Invoke1(
        std::uint32_t object, std::uint32_t slot,
        std::uint32_t first) = 0;
    [[nodiscard]] virtual std::uint32_t Invoke2(
        std::uint32_t object, std::uint32_t slot,
        std::uint32_t first, std::uint32_t second) = 0;
};

class PropertyForwardingRuntimeStub final : public IPropertyForwardingRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t Invoke0(std::uint32_t object,
                          std::uint32_t slot) override;
    std::uint32_t Invoke1(std::uint32_t object, std::uint32_t slot,
                          std::uint32_t first) override;
    std::uint32_t Invoke2(std::uint32_t object, std::uint32_t slot,
                          std::uint32_t first,
                          std::uint32_t second) override;
};

class PropertyForwarding final {
public:
    // IDA 0x5eec00 / 0x5eec10 / 0x5eec20 / 0x5eec30.
    [[nodiscard]] static std::uint32_t GetValue(
        std::uint32_t context, std::uint32_t property,
        IPropertyForwardingRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetValue(
        std::uint32_t context, std::uint32_t property,
        IPropertyForwardingRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetValue2(
        std::uint32_t context, std::uint32_t property,
        IPropertyForwardingRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetValue2(
        std::uint32_t context, std::uint32_t property,
        IPropertyForwardingRuntime& runtime);
    // IDA 0x5eec40 / 0x5eec60 / 0x5eec80 / 0x5eec90 / 0x5eecb0.
    [[nodiscard]] static std::uint32_t GetFromOffset(
        std::uint32_t wrapper, std::uint32_t offset,
        IPropertyForwardingRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetFromOffset(
        std::uint32_t wrapper, std::uint32_t offset,
        std::uint32_t value, IPropertyForwardingRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetFromVtable(
        std::uint32_t wrapper, IPropertyForwardingRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetFromOffset2(
        std::uint32_t wrapper, std::uint32_t offset,
        IPropertyForwardingRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetFromOffset2(
        std::uint32_t wrapper, std::uint32_t offset,
        std::uint32_t value, IPropertyForwardingRuntime& runtime);
};

}  // namespace pal4
