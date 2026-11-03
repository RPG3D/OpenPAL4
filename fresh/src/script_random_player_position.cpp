#include "pal4/script_random_player_position.h"

#include "pal4/player_manager_object.h"

#include <bit>

namespace pal4 {
namespace {

constexpr float kCollisionHeight = 100.0f;
constexpr float kWidthScale = std::bit_cast<float>(0x3eb33333u);
constexpr float kHeightScale = std::bit_cast<float>(0x4021eb85u);

struct Candidate {
    std::array<float, 3> position;
    ScriptCollisionProbe probe;
};

Candidate GenerateCandidate(std::uint32_t player, float minimum,
                            float maximum,
                            IScriptRandomPlayerPositionRuntime& runtime) {
    const auto x = static_cast<float>(runtime.RandomComponent(0.0f));
    const auto y = static_cast<float>(runtime.RandomComponent(0.0f));
    const auto z = static_cast<float>(runtime.RandomComponent(0.0f));
    const auto radius = runtime.RandomRadius(minimum, maximum, 0.0f);
    const auto scaled_y = static_cast<float>(static_cast<double>(y) * radius);
    const auto scaled_z = static_cast<float>(static_cast<double>(z) * radius);
    Candidate result;
    result.position = {
        static_cast<float>(static_cast<double>(runtime.ReadFloat(player + 0xacu)) +
                           static_cast<double>(x) * radius),
        static_cast<float>(static_cast<double>(runtime.ReadFloat(player + 0xb0u)) +
                           scaled_y),
        static_cast<float>(static_cast<double>(runtime.ReadFloat(player + 0xb4u)) +
                           scaled_z),
    };
    const auto& p = result.position;
    result.probe.segment = {p[0], p[1] + kCollisionHeight, p[2],
                            p[0], p[1] - kCollisionHeight, p[2]};
    return result;
}

}  // namespace

std::uint32_t ScriptRandomPlayerPosition::CheckCollision(
    std::uint32_t object, ScriptCollisionProbe& probe,
    IScriptRandomPlayerPositionRuntime& runtime) {
    if (object + 4u == 0u || runtime.ReadWord(object + 4u) != 1u)
        return object;
    std::array<std::uint8_t, 24> scratch{};
    if (runtime.ProcessClumpCollision(
            probe.segment, runtime.ReadWord(object + 0x44u), scratch) == 0u)
        return object;
    probe.blocked = 1u;
    return 0u;
}

std::uint32_t ScriptRandomPlayerPosition::MoveIndexed(
    std::int32_t player_index, float minimum, float maximum,
    IScriptRandomPlayerPositionRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_index);
    return Move(player, minimum, maximum, runtime);
}

std::uint32_t ScriptRandomPlayerPosition::MoveCurrent(
    float minimum, float maximum,
    IScriptRandomPlayerPositionRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + index * PlayerManagerObject::kElementBytes;
    return Move(player, minimum, maximum, runtime);
}

std::uint32_t ScriptRandomPlayerPosition::Move(
    std::uint32_t player, float minimum, float maximum,
    IScriptRandomPlayerPositionRuntime& runtime) {
    if (player == 0u) return 0u;
    auto candidate = GenerateCandidate(player, minimum, maximum, runtime);
    runtime.ForEachCollisionObject(runtime.CollisionWorld(), candidate.probe);
    const auto width = static_cast<float>(
        runtime.ObjectProperty(player) * kWidthScale);
    const auto height = static_cast<float>(
        runtime.ObjectProperty(player) * kHeightScale);

    for (std::uint32_t remaining = 100u;;) {
        if (runtime.CheckMovement(runtime.MovementAdapter(),
                                  candidate.position, width, height) &&
            candidate.probe.blocked == 0u) {
            runtime.SetTransform(player, candidate.position, 0u);
            return runtime.UpdateGrid(runtime.MinimapManager(),
                                      player + 0xacu, player + 0xa0u);
        }
        if (--remaining == 0u) return 0u;
        candidate = GenerateCandidate(player, minimum, maximum, runtime);
        runtime.ForEachCollisionObject(runtime.CollisionWorld(),
                                       candidate.probe);
    }
}

}  // namespace pal4
