#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {

struct GameObjectPoint {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// The ordered node cursor preserves the original live STL tree traversal.
class IPalGameObjectManagerFrameRuntime {
public:
    virtual ~IPalGameObjectManagerFrameRuntime()=default;
    virtual std::uint32_t FirstNode(std::uint32_t)=0;
    virtual std::uint32_t EndNode(std::uint32_t)=0;
    virtual std::uint32_t NextNode(std::uint32_t)=0;
    virtual std::uint32_t NodeObject(std::uint32_t)=0;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual void UpdateObject(std::uint32_t,std::uint32_t)=0;
};
class IPalGameObjectManagerCleanupRuntime {
public:
    virtual ~IPalGameObjectManagerCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FirstNode(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t EndNode(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t NodeObject(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t ClearObjectTree(std::uint32_t manager) = 0;
    virtual void ReleaseObjectResources(std::uint32_t object) = 0;
    virtual void DestroyObject(std::uint32_t object, std::uint32_t mode) = 0;
};

class IPalGameObjectManagerVisitRuntime : public IPalGameObjectManagerCleanupRuntime {
public:
    ~IPalGameObjectManagerVisitRuntime() override = default;
    [[nodiscard]] virtual std::string_view ObjectName(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DeactivateObject(std::uint32_t object) = 0;
    virtual void ActivateObject(std::uint32_t object) = 0;
    virtual void UpdateObject(std::uint32_t object,
                              std::uint32_t argument) = 0;
    virtual void UpdateObjectSecond(std::uint32_t object,
                                    std::uint32_t argument) = 0;
    [[nodiscard]] virtual bool IsSelectable(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool TryPointDistance(
        std::uint32_t object, const GameObjectPoint& point,
        float& distance) = 0;
    [[nodiscard]] virtual std::uint32_t MaterialCount(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::int32_t ObjectType(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual bool TryRayDistance(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, float& distance) = 0;
};

class PalGameObjectManagerVisit final {
public:
    using Visitor = std::function<std::uint32_t(
        std::uint32_t object, std::uint32_t context)>;

    [[nodiscard]] static std::uint32_t DeactivateAndClear(
        std::uint32_t manager, IPalGameObjectManagerCleanupRuntime& runtime);
    [[nodiscard]] static std::uint32_t VisitUntilFalse(
        std::uint32_t manager, const Visitor& visitor,
        std::uint32_t context, IPalGameObjectManagerVisitRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateEnabled(
        std::uint32_t manager, std::uint32_t argument,
        IPalGameObjectManagerVisitRuntime& runtime);
    static std::uint32_t UpdateEnabled(std::uint32_t,std::uint32_t,IPalGameObjectManagerFrameRuntime&);
    [[nodiscard]] static std::uint32_t UpdateEnabledSecond(
        std::uint32_t manager, std::uint32_t argument,
        IPalGameObjectManagerVisitRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindClosestPoint(
        std::uint32_t manager, const GameObjectPoint& point,
        bool bypass_gate, bool mark_selected,
        IPalGameObjectManagerVisitRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindClosestRay(
        std::uint32_t manager, std::uint32_t first, std::uint32_t second,
        bool mark_selected, std::int32_t type,
        IPalGameObjectManagerVisitRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindByName(
        std::uint32_t manager, std::string_view name,
        IPalGameObjectManagerVisitRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeactivateAll(
        std::uint32_t manager, IPalGameObjectManagerVisitRuntime& runtime);
    [[nodiscard]] static std::uint32_t ActivateAll(
        std::uint32_t manager, IPalGameObjectManagerVisitRuntime& runtime);
};

}  // namespace pal4
