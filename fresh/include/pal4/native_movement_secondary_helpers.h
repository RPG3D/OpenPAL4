#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/pal_movement_secondary_collection.h"
#include "pal4/pal_movement_secondary_collision.h"

#include <unordered_map>

struct RpClump;

namespace pal4 {

// Typed native endpoints for the original secondary helper's virtual slots.
// The helper and its clump remain owned by the scene GOB.
class INativeMovementSecondaryHelper {
public:
    virtual ~INativeMovementSecondaryHelper() = default;
    [[nodiscard]] virtual RpClump* Clump() const = 0;
    [[nodiscard]] virtual double SwitchThreshold() const = 0;
    virtual void Enter() = 0;
    virtual void Leave() = 0;
};

// Native collection/collision endpoints for IDA 429260/429490/4295C0/4297A0.
// This owns only helper-address storage; no clumps or game helper objects.
class NativeMovementSecondaryHelpers final
    : private IPalMovementSecondaryCollectionRuntime,
      private IPalMovementSecondaryCollisionRuntime {
public:
    NativeMovementSecondaryHelpers();
    NativeMovementSecondaryHelpers(const NativeMovementSecondaryHelpers&) = delete;
    NativeMovementSecondaryHelpers& operator=(const NativeMovementSecondaryHelpers&) = delete;
    ~NativeMovementSecondaryHelpers() override = default;

    [[nodiscard]] bool Add(std::uint32_t identity, INativeMovementSecondaryHelper& helper);
    [[nodiscard]] bool Remove(std::uint32_t identity);
    [[nodiscard]] bool FindHighest(const std::array<float, 3>& position,
        float above, float below, std::array<float, 3>* output,
        bool store_atomic = true, bool store_triangle = true);
    [[nodiscard]] bool HasCollision(const std::array<float, 3>& position,
        float above, float below, bool check_height);
    [[nodiscard]] bool UpdateActive(const std::array<float, 3>& position,
        float above, float below);
    void ClearActive();
    [[nodiscard]] RpClump* ActiveClump();
    [[nodiscard]] const PalMovementSecondaryChildState& State() const noexcept { return state_; }
    [[nodiscard]] std::size_t Count() const noexcept { return helpers_.size(); }

private:
    [[nodiscard]] INativeMovementSecondaryHelper& Helper(std::uint32_t identity) const;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t ReadElement(std::uint32_t address) override;
    std::uint32_t GetClump(std::uint32_t element) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    bool QueryCollisionData(std::uint32_t clump) override;
    bool ProcessClumpCollision(const std::array<float, 6>& line,
        std::uint32_t clump, PalMovementCollisionCandidate& candidate) override;
    double SwitchThreshold(std::uint32_t element) override;
    void Leave(std::uint32_t element) override;
    void Enter(std::uint32_t element) override;

    NativeBaseObjectMemory memory_;
    PalMovementSecondaryChildState state_{};
    std::unordered_map<std::uint32_t, INativeMovementSecondaryHelper*> helpers_;
};

}  // namespace pal4
