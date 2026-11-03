#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct MonsterInteractionObject {
    std::array<float, 3> position{};
    std::array<float, 3> collision_position{};
};

struct MonsterInteractionContext : MonsterInteractionObject {
    std::uint32_t target_address{};
    std::uint32_t source_address{};
};

class IMonsterObjectInteractionRuntime {
public:
    virtual ~IMonsterObjectInteractionRuntime() = default;
    [[nodiscard]] virtual MonsterInteractionContext Context(
        std::uint32_t manager_address) = 0;
    [[nodiscard]] virtual MonsterInteractionObject Candidate(
        std::uint32_t candidate_address) = 0;
    [[nodiscard]] virtual double TargetDoubleProperty(
        std::uint32_t target_address) = 0;
    [[nodiscard]] virtual double PlanarDistance(float dx, float dz) = 0;
    [[nodiscard]] virtual float InteractionRadius(
        std::uint32_t source_address) = 0;
    [[nodiscard]] virtual std::uint32_t CollisionClump() = 0;
    [[nodiscard]] virtual bool CollisionBlocked(
        const std::array<float, 6>& endpoints,
        std::uint32_t clump_address) = 0;
    virtual void EnterPursuit(std::uint32_t candidate_address,
                              std::uint32_t target_address) = 0;
};

class MonsterObjectInteractionRuntimeStub final
    : public IMonsterObjectInteractionRuntime {
public:
    MonsterInteractionContext Context(std::uint32_t manager_address) override;
    MonsterInteractionObject Candidate(
        std::uint32_t candidate_address) override;
    double TargetDoubleProperty(std::uint32_t target_address) override;
    double PlanarDistance(float dx, float dz) override;
    float InteractionRadius(std::uint32_t source_address) override;
    std::uint32_t CollisionClump() override;
    bool CollisionBlocked(const std::array<float, 6>& endpoints,
                          std::uint32_t clump_address) override;
    void EnterPursuit(std::uint32_t candidate_address,
                      std::uint32_t target_address) override;
};

class MonsterObjectInteractionCallback final {
public:
    // IDA 0x434600: collection callback used by MonsterActionDispatcher.
    [[nodiscard]] static std::uint32_t Process(
        std::uint32_t candidate_address, std::uint32_t manager_address,
        IMonsterObjectInteractionRuntime& runtime);
};

}  // namespace pal4
