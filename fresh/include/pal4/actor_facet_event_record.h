#pragma once

#include <cstdint>

namespace pal4 {

class IActorFacetEventRecordRuntime {
public:
    virtual ~IActorFacetEventRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ResetTemporaryHandle(std::uint32_t& first,
                                      std::uint32_t& second) = 0;
};

class ActorFacetEventRecord final {
public:
    static std::uint32_t ActorFromFacetValue(std::uint32_t facet);
    static constexpr std::uint32_t kActorFacetOffset = 716u;

    // IDA 0x562e90/0x562ea0/0x562ed0.
    [[nodiscard]] static std::uint32_t ConstructRaw(
        std::uint32_t record, std::uint32_t value,
        IActorFacetEventRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructFacet(
        std::uint32_t record, std::uint32_t actor,
        IActorFacetEventRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t ActorFromFacet(
        std::uint32_t record, IActorFacetEventRecordRuntime& runtime);

    // IDA 0x562d70/0x562dd0/0x562e00/0x562ee0/0x562f00.
    [[nodiscard]] static std::uint32_t ConstructFourWords(
        std::uint32_t record, std::uint32_t actor,
        std::uint32_t second, std::uint32_t third,
        std::uint32_t fourth, IActorFacetEventRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructFixedMode(
        std::uint32_t record, std::uint32_t actor,
        IActorFacetEventRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructPair(
        std::uint32_t record, std::uint32_t actor,
        std::uint32_t second, std::uint32_t third,
        IActorFacetEventRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructOneExtra(
        std::uint32_t record, std::uint32_t actor,
        std::uint32_t second, IActorFacetEventRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructWithEffectHandle(
        std::uint32_t record, std::uint32_t actor,
        std::uint32_t handle_first, std::uint32_t handle_second,
        IActorFacetEventRecordRuntime& runtime);
};

}  // namespace pal4
