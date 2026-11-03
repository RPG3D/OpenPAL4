#pragma once

#include <cstdint>

namespace pal4 {

class IFadeOffTaskRuntime {
public:
    virtual ~IFadeOffTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void SetActorState(std::uint32_t actor,
                               std::uint32_t state,
                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t FloatToLong(float value) = 0;
    [[nodiscard]] virtual std::uint32_t ActorFacet(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t SetFacetColor(
        std::uint32_t facet, std::uint32_t color) = 0;
};

class FadeOffTask final {
public:
    // IDA 0x55aed0 and 0x55af20.
    [[nodiscard]] static std::uint8_t Start(
        std::uint32_t task, IFadeOffTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateAlpha(
        std::uint32_t task, float step, IFadeOffTaskRuntime& runtime);
};

}  // namespace pal4
