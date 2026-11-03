#include "pal4/pal_game_object_manager_visit.h"

#include <limits>

namespace pal4 {

std::uint32_t PalGameObjectManagerVisit::DeactivateAndClear(
    const std::uint32_t manager, IPalGameObjectManagerCleanupRuntime& runtime) {
    const auto end = runtime.EndNode(manager);
    for (auto node = runtime.FirstNode(manager); node != end;
         node = runtime.NextNode(node)) {
        const auto object = runtime.NodeObject(node);
        runtime.ReleaseObjectResources(object);
        runtime.DestroyObject(object, 1u);
    }
    return runtime.ClearObjectTree(manager);
}

std::uint32_t PalGameObjectManagerVisit::VisitUntilFalse(
    const std::uint32_t manager, const Visitor& visitor,
    const std::uint32_t context,
    IPalGameObjectManagerVisitRuntime& runtime) {
    const auto end = runtime.EndNode(manager);
    for (auto node = runtime.FirstNode(manager); node != end;
         node = runtime.NextNode(node)) {
        if (visitor(runtime.NodeObject(node), context) == 0u) return 0u;
    }
    return end;
}

std::uint32_t PalGameObjectManagerVisit::UpdateEnabled(std::uint32_t manager,std::uint32_t argument,IPalGameObjectManagerVisitRuntime& runtime){
    class Frame final : public IPalGameObjectManagerFrameRuntime {
        IPalGameObjectManagerVisitRuntime& runtime;
    public:
        explicit Frame(IPalGameObjectManagerVisitRuntime& r):runtime(r){}
        std::uint32_t FirstNode(std::uint32_t a)override{return runtime.FirstNode(a);}
        std::uint32_t EndNode(std::uint32_t a)override{return runtime.EndNode(a);}
        std::uint32_t NextNode(std::uint32_t a)override{return runtime.NextNode(a);}
        std::uint32_t NodeObject(std::uint32_t a)override{return runtime.NodeObject(a);}
        std::uint32_t ReadWord(std::uint32_t a)override{return runtime.ReadWord(a);}
        void UpdateObject(std::uint32_t o,std::uint32_t c)override{runtime.UpdateObject(o,c);}
    } frame(runtime);return UpdateEnabled(manager,argument,frame);
}
std::uint32_t PalGameObjectManagerVisit::UpdateEnabled(
    const std::uint32_t manager, const std::uint32_t argument,
    IPalGameObjectManagerFrameRuntime& runtime) {
    const auto end = runtime.EndNode(manager);
    for (auto node = runtime.FirstNode(manager); node != end;
         node = runtime.NextNode(node)) {
        const auto object = runtime.NodeObject(node);
        if (runtime.ReadWord(object + 0x184u) != 0u)
            runtime.UpdateObject(object, argument);
    }
    return end;
}

std::uint32_t PalGameObjectManagerVisit::UpdateEnabledSecond(
    const std::uint32_t manager, const std::uint32_t argument,
    IPalGameObjectManagerVisitRuntime& runtime) {
    const auto end = runtime.EndNode(manager);
    for (auto node = runtime.FirstNode(manager); node != end;
         node = runtime.NextNode(node)) {
        const auto object = runtime.NodeObject(node);
        if (runtime.ReadWord(object + 0x184u) != 0u)
            runtime.UpdateObjectSecond(object, argument);
    }
    return end;
}

std::uint32_t PalGameObjectManagerVisit::FindClosestPoint(
    const std::uint32_t manager, const GameObjectPoint& point,
    const bool bypass_gate, const bool mark_selected,
    IPalGameObjectManagerVisitRuntime& runtime) {
    auto closest = 0u;
    auto best_distance = std::numeric_limits<float>::max();
    const auto end = runtime.EndNode(manager);
    for (auto node = runtime.FirstNode(manager); node != end;
         node = runtime.NextNode(node)) {
        const auto object = runtime.NodeObject(node);
        runtime.WriteWord(object + 0x1c8u, 0u);
        if (!bypass_gate &&
            (runtime.ReadWord(object + 0x184u) == 0u ||
             !runtime.IsSelectable(object)))
            continue;
        float distance = 0.0f;
        if (runtime.TryPointDistance(object, point, distance) &&
            distance < best_distance) {
            closest = object;
            best_distance = distance;
        }
    }
    if (mark_selected && closest != 0u)
        runtime.WriteWord(closest + 0x1c8u, 1u);
    return closest;
}

std::uint32_t PalGameObjectManagerVisit::FindClosestRay(
    const std::uint32_t manager, const std::uint32_t first,
    const std::uint32_t second, const bool mark_selected,
    const std::int32_t type, IPalGameObjectManagerVisitRuntime& runtime) {
    auto closest = 0u;
    auto best_distance = std::numeric_limits<float>::max();
    const auto end = runtime.EndNode(manager);
    for (auto node = runtime.FirstNode(manager); node != end;
         node = runtime.NextNode(node)) {
        const auto object = runtime.NodeObject(node);
        const auto clump = runtime.ReadWord(object + 0x14u);
        runtime.WriteWord(object + 0x1c8u, 0u);
        if (runtime.MaterialCount(clump) == 0u ||
            (type >= 0 && runtime.ObjectType(object) != type) ||
            runtime.ReadWord(object + 0x184u) == 0u ||
            !runtime.IsSelectable(object))
            continue;
        float distance = 0.0f;
        if (runtime.TryRayDistance(object, first, second, distance) &&
            distance < best_distance) {
            closest = object;
            best_distance = distance;
        }
    }
    if (mark_selected && closest != 0u)
        runtime.WriteWord(closest + 0x1c8u, 1u);
    return closest;
}

std::uint32_t PalGameObjectManagerVisit::FindByName(
    const std::uint32_t manager, const std::string_view name,
    IPalGameObjectManagerVisitRuntime& runtime) {
    const auto end = runtime.EndNode(manager);
    for (auto node = runtime.FirstNode(manager); node != end;
         node = runtime.NextNode(node)) {
        const auto object = runtime.NodeObject(node);
        if (runtime.ObjectName(object) == name) return object;
    }
    return 0u;
}

std::uint32_t PalGameObjectManagerVisit::DeactivateAll(
    const std::uint32_t manager,
    IPalGameObjectManagerVisitRuntime& runtime) {
    const auto end = runtime.EndNode(manager);
    for (auto node = runtime.FirstNode(manager); node != end;
         node = runtime.NextNode(node))
        runtime.DeactivateObject(runtime.NodeObject(node));
    return end;
}

std::uint32_t PalGameObjectManagerVisit::ActivateAll(
    const std::uint32_t manager,
    IPalGameObjectManagerVisitRuntime& runtime) {
    const auto end = runtime.EndNode(manager);
    for (auto node = runtime.FirstNode(manager); node != end;
         node = runtime.NextNode(node))
        runtime.ActivateObject(runtime.NodeObject(node));
    return end;
}

}  // namespace pal4
