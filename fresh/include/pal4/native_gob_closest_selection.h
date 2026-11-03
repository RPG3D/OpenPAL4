#pragma once

#include "pal4/pal_actor_control_interaction.h"
#include "pal4/pal_game_object_manager_visit.h"

#include <cstdint>
#include <vector>

namespace pal4 {

class NativeBaseObjectMemory;
class NativeSceneGobAssets;

// Adapts the live GOB name tree to the rewritten manager selection path.
class NativeGobClosestSelection final : private IPalGameObjectManagerVisitRuntime {
public:
    NativeGobClosestSelection(NativeSceneGobAssets& assets,
                              NativeBaseObjectMemory& memory);
    [[nodiscard]] std::uint32_t FindRay(const ActorInteractionRay& ray,
                                        bool mark_selected);
    [[nodiscard]] std::uint32_t FindPoint(const GameObjectPoint& point,
                                          float radius, bool mark_selected);
    [[nodiscard]] bool Enabled(std::uint32_t object) const;
    [[nodiscard]] bool InRange(std::uint32_t object,
                               std::uint32_t position, float radius) const;

private:
    std::uint32_t FirstNode(std::uint32_t manager) override;
    std::uint32_t EndNode(std::uint32_t manager) override;
    std::uint32_t NextNode(std::uint32_t node) override;
    std::uint32_t NodeObject(std::uint32_t node) override;
    std::string_view ObjectName(std::uint32_t object) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t ClearObjectTree(std::uint32_t manager) override;
    void ReleaseObjectResources(std::uint32_t object) override;
    void DestroyObject(std::uint32_t object, std::uint32_t mode) override;
    void DeactivateObject(std::uint32_t object) override;
    void ActivateObject(std::uint32_t object) override;
    void UpdateObject(std::uint32_t object, std::uint32_t argument) override;
    void UpdateObjectSecond(std::uint32_t object,
                            std::uint32_t argument) override;
    bool IsSelectable(std::uint32_t object) override;
    bool TryPointDistance(std::uint32_t object,
                          const GameObjectPoint& point,
                          float& distance) override;
    std::uint32_t MaterialCount(std::uint32_t clump) override;
    std::int32_t ObjectType(std::uint32_t object) override;
    bool TryRayDistance(std::uint32_t object, std::uint32_t first,
                        std::uint32_t second, float& distance) override;

    [[nodiscard]] bool WithinBounds(std::uint32_t object,
                                     const float* position, float radius,
                                     float* distance) const;
    NativeSceneGobAssets& assets_;
    NativeBaseObjectMemory& memory_;
    std::vector<std::uint32_t> objects_;
};

}  // namespace pal4
