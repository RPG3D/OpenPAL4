#pragma once

#include <cstdint>

namespace pal4 {

class IEffectRandomRuntime {
public:
    virtual ~IEffectRandomRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Singleton() = 0;
    virtual void SetSingleton(std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentTime() = 0;
    virtual void FreeObject(std::uint32_t object) = 0;
};

class EffectRandom final {
public:
    static constexpr std::uint32_t kVtable = 0x84b234u;
    static constexpr std::uint32_t kBaseVtable = 0x84b238u;

    // IDA 0x63cce0/0x63cd70: register or clear the global random object.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t seed,
        IEffectRandomRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IEffectRandomRuntime& runtime);
    // IDA 0x63d510: deleting destructor uses the low flag bit.
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags,
        IEffectRandomRuntime& runtime);
    // IDA 0x63cdc0..0x63ce20: LCG, unit value and x87 range wrapper.
    [[nodiscard]] static std::uint32_t NextBounded(
        std::uint32_t object, std::uint32_t bound,
        IEffectRandomRuntime& runtime);
    [[nodiscard]] static double NextUnit(
        std::uint32_t object, IEffectRandomRuntime& runtime);
    [[nodiscard]] static double NextRange(
        std::uint32_t object, float minimum, float maximum,
        IEffectRandomRuntime& runtime);
};

}  // namespace pal4
