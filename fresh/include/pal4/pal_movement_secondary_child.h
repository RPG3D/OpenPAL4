#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IPalMovementSecondaryCollisionRuntime;
class IPalMovementSecondaryCollectionRuntime;

struct PalMovementSecondaryChildState {
    std::array<std::uint32_t, 34> words{};
};
static_assert(sizeof(PalMovementSecondaryChildState) == 0x88u);

class IPalMovementSecondaryChildRuntime {
public:
    virtual ~IPalMovementSecondaryChildRuntime() = default;
    virtual void InitializeConfig(PalMovementSecondaryChildState& state) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupConfig(
        PalMovementSecondaryChildState& state) = 0;
    virtual void CleanupAlternate(PalMovementSecondaryChildState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ReadElement(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetClump(
        std::uint32_t element) = 0;
    virtual void ResetAtomicCallbacks(std::uint32_t clump) = 0;
};

class PalMovementSecondaryChildRuntimeStub final
    : public IPalMovementSecondaryChildRuntime {
public:
    void InitializeConfig(PalMovementSecondaryChildState& state) override;
    void Free(std::uint32_t address) override;
    std::uint32_t CleanupConfig(
        PalMovementSecondaryChildState& state) override;
    void CleanupAlternate(PalMovementSecondaryChildState& state) override;
    std::uint32_t ReadElement(std::uint32_t address) override;
    std::uint32_t GetClump(std::uint32_t element) override;
    void ResetAtomicCallbacks(std::uint32_t clump) override;
};

class PalMovementSecondaryChild final {
public:
    static constexpr std::uint32_t kVtable = 0x8427a0u;
    static constexpr std::uint32_t kBaseVtable = 0x842760u;

    [[nodiscard]] static PalMovementSecondaryChildState& Construct(
        PalMovementSecondaryChildState& state,
        std::uint8_t observed_stack_byte,
        IPalMovementSecondaryChildRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadField80(
        const PalMovementSecondaryChildState& state);
    [[nodiscard]] static std::uint32_t ReadField84(
        const PalMovementSecondaryChildState& state);
    [[nodiscard]] static std::uint32_t Delete(
        PalMovementSecondaryChildState& state, std::uint32_t address,
        std::uint32_t flags, IPalMovementSecondaryChildRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        PalMovementSecondaryChildState& state,
        IPalMovementSecondaryChildRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearClumpCallbacks(
        PalMovementSecondaryChildState& state,
        IPalMovementSecondaryChildRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindHighestCollision(
        PalMovementSecondaryChildState& state,
        std::uint32_t write_field84, std::uint32_t write_field80,
        const std::array<float, 3>& position,
        float upward_distance, float downward_distance,
        std::array<float, 3>* output_position,
        IPalMovementSecondaryCollisionRuntime& runtime);
    [[nodiscard]] static std::uint32_t HasCollisionWithinHeight(
        const PalMovementSecondaryChildState& state,
        const std::array<float, 3>& position,
        float upward_distance, float downward_distance,
        std::uint32_t check_height,
        IPalMovementSecondaryCollisionRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateActiveCollision(
        PalMovementSecondaryChildState& state,
        const std::array<float, 3>& position,
        float upward_distance, float downward_distance,
        IPalMovementSecondaryCollisionRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddClump(
        PalMovementSecondaryChildState& state, std::uint32_t element,
        IPalMovementSecondaryCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveClump(
        PalMovementSecondaryChildState& state, std::uint32_t element,
        IPalMovementSecondaryCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetActiveClump(
        const PalMovementSecondaryChildState& state,
        IPalMovementSecondaryCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearActiveClump(
        PalMovementSecondaryChildState& state,
        IPalMovementSecondaryCollectionRuntime& runtime);
};

}  // namespace pal4
