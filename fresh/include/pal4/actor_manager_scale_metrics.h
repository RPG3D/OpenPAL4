#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IActorManagerScaleMetricsRuntime {
public:
    virtual ~IActorManagerScaleMetricsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
};

class ActorManagerScaleMetrics final {
public:
    [[nodiscard]] static std::array<float, 3> Scale(
        std::uint32_t actor, IActorManagerScaleMetricsRuntime& runtime);
    [[nodiscard]] static double ClampedSize(
        std::uint32_t actor, IActorManagerScaleMetricsRuntime& runtime);
};

}  // namespace pal4
