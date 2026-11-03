#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct ScriptCollisionProbe {
    std::array<float, 6> segment{};
    std::uint32_t blocked = 0u;
};

class IScriptRandomPlayerPositionRuntime {
public:
    virtual ~IScriptRandomPlayerPositionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual double RandomComponent(float zero) = 0;
    [[nodiscard]] virtual double RandomRadius(
        float minimum, float maximum, float zero) = 0;
    [[nodiscard]] virtual std::uint32_t CollisionWorld() = 0;
    virtual void ForEachCollisionObject(
        std::uint32_t world, ScriptCollisionProbe& probe) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessClumpCollision(
        const std::array<float, 6>& segment, std::uint32_t clump,
        std::array<std::uint8_t, 24>& scratch) = 0;
    [[nodiscard]] virtual double ObjectProperty(std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t MovementAdapter() = 0;
    [[nodiscard]] virtual bool CheckMovement(
        std::uint32_t adapter, std::array<float, 3>& candidate,
        float width, float height) = 0;
    virtual void SetTransform(std::uint32_t player,
                              const std::array<float, 3>& position,
                              std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t MinimapManager() = 0;
    [[nodiscard]] virtual std::uint32_t UpdateGrid(
        std::uint32_t minimap, std::uint32_t position,
        std::uint32_t orientation) = 0;
};

class ScriptRandomPlayerPosition final {
public:
    // IDA 0x5d6150, collision callback for both random-position handlers.
    [[nodiscard]] static std::uint32_t CheckCollision(
        std::uint32_t object, ScriptCollisionProbe& probe,
        IScriptRandomPlayerPositionRuntime& runtime);
    // IDA 0x5d61a0, giPlayerRandomPosition.
    [[nodiscard]] static std::uint32_t MoveIndexed(
        std::int32_t player_index, float minimum, float maximum,
        IScriptRandomPlayerPositionRuntime& runtime);
    // IDA 0x5d6400, giPlayerCurrentRandomPosition.
    [[nodiscard]] static std::uint32_t MoveCurrent(
        float minimum, float maximum,
        IScriptRandomPlayerPositionRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t Move(
        std::uint32_t player, float minimum, float maximum,
        IScriptRandomPlayerPositionRuntime& runtime);
};

}  // namespace pal4
