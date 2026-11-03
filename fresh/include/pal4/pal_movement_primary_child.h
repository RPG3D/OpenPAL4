#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IPalMovementPrimaryCollisionRuntime;

struct PalMovementPrimaryChildState {
    std::array<std::uint32_t, 29> words{};
};
static_assert(sizeof(PalMovementPrimaryChildState) == 0x74u);

class IPalMovementPrimaryChildRuntime {
public:
    virtual ~IPalMovementPrimaryChildRuntime() = default;
    virtual void InitializeConfig(PalMovementPrimaryChildState& state) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupConfig2(
        PalMovementPrimaryChildState& state) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupConfig3(
        PalMovementPrimaryChildState& state) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class PalMovementPrimaryChildRuntimeStub final
    : public IPalMovementPrimaryChildRuntime {
public:
    void InitializeConfig(PalMovementPrimaryChildState& state) override;
    std::uint32_t CleanupConfig2(
        PalMovementPrimaryChildState& state) override;
    std::uint32_t CleanupConfig3(
        PalMovementPrimaryChildState& state) override;
    void Free(std::uint32_t address) override;
};

class PalMovementPrimaryChild final {
public:
    static constexpr std::uint32_t kVtable = 0x84272cu;
    static constexpr std::uint32_t kBaseVtable = 0x842760u;

    [[nodiscard]] static PalMovementPrimaryChildState& Construct(
        PalMovementPrimaryChildState& state,
        IPalMovementPrimaryChildRuntime& runtime);
    [[nodiscard]] static std::uint32_t AlwaysReady(
        const PalMovementPrimaryChildState& state,
        std::uint32_t unused_first, std::uint32_t unused_second,
        std::uint32_t unused_third);
    [[nodiscard]] static std::uint32_t ReadField34(
        const PalMovementPrimaryChildState& state);
    [[nodiscard]] static std::uint32_t ReadField6C(
        const PalMovementPrimaryChildState& state);
    [[nodiscard]] static std::uint32_t ReadField70(
        const PalMovementPrimaryChildState& state);
    [[nodiscard]] static std::uint32_t Delete(
        PalMovementPrimaryChildState& state, std::uint32_t address,
        std::uint32_t flags, IPalMovementPrimaryChildRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        PalMovementPrimaryChildState& state,
        IPalMovementPrimaryChildRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupAlternate(
        PalMovementPrimaryChildState& state,
        IPalMovementPrimaryChildRuntime& runtime);
    [[nodiscard]] static std::uint32_t CheckIntersectionOptions(
        std::uint32_t object, std::uint32_t use_field70,
        std::uint32_t use_field6c, std::uint32_t first,
        float second, float third, std::uint32_t fourth,
        IPalMovementPrimaryCollisionRuntime& runtime);
    [[nodiscard]] static std::uint32_t ValidateIntersectionHeight(
        std::uint32_t source, float second, float third,
        std::uint32_t check_height,
        IPalMovementPrimaryCollisionRuntime& runtime);
};

class PalMovementBaseChild final {
public:
    [[nodiscard]] static std::uint32_t Delete(
        PalMovementPrimaryChildState& state, std::uint32_t address,
        std::uint32_t flags, IPalMovementPrimaryChildRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        PalMovementPrimaryChildState& state,
        IPalMovementPrimaryChildRuntime& runtime);
};

}  // namespace pal4
