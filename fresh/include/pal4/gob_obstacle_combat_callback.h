#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IGobObstacleCombatCallbackRuntime {
public:
    virtual ~IGobObstacleCombatCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t MovementAdapter() = 0;
    virtual void InvokeObjectSlot80(
        std::uint32_t object,
        const std::array<std::uint32_t, 3>& data) = 0;
    virtual void ApplyEffectTypeToActor(
        std::uint32_t object, std::uint32_t context) = 0;
};

class GobObstacleCombatCallback final {
public:
    // IDA 0x5a9bb0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object,
        IGobObstacleCombatCallbackRuntime& runtime);
    // IDA 0x5a9bf0: abstract callback-base constructor returns zero.
    [[nodiscard]] static std::uint32_t ConstructBase(
        std::uint32_t object,
        IGobObstacleCombatCallbackRuntime& runtime);
    // IDA 0x5a9c00: context is carried in EBP by the original call path.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t callback, std::uint32_t context,
        std::uint32_t argument,
        IGobObstacleCombatCallbackRuntime& runtime);
};

}  // namespace pal4
