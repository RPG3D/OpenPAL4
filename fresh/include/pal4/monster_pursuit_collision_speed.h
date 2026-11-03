#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct MonsterBehaviorManagerState;

struct MonsterPursuitTarget {
    bool active{};
    std::array<float, 3> position{};
};

class IMonsterPursuitSpeedRuntime {
public:
    virtual ~IMonsterPursuitSpeedRuntime() = default;
    [[nodiscard]] virtual MonsterPursuitTarget Target(
        std::uint32_t target_address) = 0;
    [[nodiscard]] virtual std::uint32_t FindPursuitPath(
        float elapsed, std::array<float, 3>& target_position) = 0;
    [[nodiscard]] virtual bool ApplyPursuitPath(
        std::uint32_t movement_address,
        const std::array<float, 3>& manager_position,
        const std::array<float, 3>& target_position,
        std::uint32_t path_address, std::uint32_t argument) = 0;
    [[nodiscard]] virtual float MaxPursuitSpeed(
        std::uint32_t source_address, float fallback) = 0;
    [[nodiscard]] virtual std::uint32_t SetPursuitSpeed(
        std::uint32_t movement_address, float speed) = 0;
};

class IMonsterPursuitReturnRuntime : public IMonsterPursuitSpeedRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ResetPursuit(
        MonsterBehaviorManagerState& state,
        std::uint32_t manager_address) = 0;
};

class MonsterPursuitSpeedRuntimeStub final : public IMonsterPursuitReturnRuntime {
public:
    MonsterPursuitTarget Target(std::uint32_t target_address) override;
    std::uint32_t FindPursuitPath(
        float elapsed, std::array<float, 3>& target_position) override;
    bool ApplyPursuitPath(
        std::uint32_t movement_address,
        const std::array<float, 3>& manager_position,
        const std::array<float, 3>& target_position,
        std::uint32_t path_address, std::uint32_t argument) override;
    float MaxPursuitSpeed(std::uint32_t source_address,
                          float fallback) override;
    std::uint32_t SetPursuitSpeed(std::uint32_t movement_address,
                                  float speed) override;
    std::uint32_t ResetPursuit(MonsterBehaviorManagerState& state,
                                std::uint32_t manager_address) override;
};

class MonsterPursuitCollisionSpeed final {
public:
    // IDA 0x4342c0: path acceptance and configured pursuit speed.
    [[nodiscard]] static bool Process(
        const MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, float elapsed,
        std::uint32_t target_address,
        IMonsterPursuitSpeedRuntime& runtime);
};

class MonsterPursuitReturnState final {
public:
    // IDA 0x4343c0: switch to return mode and seek the saved position.
    [[nodiscard]] static std::uint32_t Enter(
        MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, float elapsed,
        IMonsterPursuitReturnRuntime& runtime);
};

}  // namespace pal4
