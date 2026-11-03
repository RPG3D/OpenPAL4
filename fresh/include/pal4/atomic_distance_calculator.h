#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using AtomicPosition = std::array<float, 3>;

class IAtomicDistanceRuntime {
public:
    virtual ~IAtomicDistanceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void ResyncInterpolatedSphere(std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t frame) = 0;
    virtual void CalculatePosition(AtomicPosition& destination,
                                   std::uint32_t source,
                                   std::uint32_t transform) = 0;
};

class AtomicDistanceRuntimeStub final : public IAtomicDistanceRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void ResyncInterpolatedSphere(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t GetObjectTransform(
        std::uint32_t) override { return 0u; }
    void CalculatePosition(AtomicPosition&, std::uint32_t,
                           std::uint32_t) override {}
};

class AtomicDistanceCalculator final {
public:
    // IDA 0x5fa2f0.
    [[nodiscard]] static double DistanceSquared(
        std::uint32_t source_atomic, std::uint32_t target_atomic,
        IAtomicDistanceRuntime& runtime);
};

}  // namespace pal4
