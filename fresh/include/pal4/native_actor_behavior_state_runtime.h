#pragma once
#include "pal4/monster_path_endpoint_restore.h"
#include <cstdint>
#include <string_view>
#include <functional>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeNpcManagerOwner;
class NativeBehaviorHelperConfiguration;

// Actual actor/animation/path owners supply these services. No default
// implementation is provided, and they must outlive a state transition.
class INativeActorBehaviorResources {
public:
    virtual ~INativeActorBehaviorResources() = default;
    virtual std::uint32_t FindPath(std::string_view name) = 0;
    virtual MonsterTransformRows Transform(std::uint32_t npc, std::uint32_t clump) = 0;
    virtual bool Select(std::uint32_t npc, std::string_view name, bool blended, float blend) = 0;
    virtual std::uint32_t SetRestart(std::uint32_t npc, std::uint32_t flag) = 0;
    virtual float Time(std::uint32_t npc) = 0;
    virtual void SetTime(std::uint32_t npc, float time) = 0;
    virtual std::uint32_t Completed(std::uint32_t npc) = 0;
};

enum class NativeBehaviorActorFamily { Npc, Monster };
struct NativeBehaviorStateGrid {
    NativeBehaviorActorFamily family;
    std::uint32_t manager;
    std::uint32_t wrapper;
    std::function<bool(std::uint32_t)> add, remove;
};

// Composes existing NPC/monster restore/save algorithms with actual native
// XML documents, helper configuration and manager pointer-list operations.
// Save's grid removal invokes the bound element deleting callback; a registered
// helper may be freed before Save returns. Never destroy/free it again.
class NativeActorBehaviorStateRuntime final {
public:
    NativeActorBehaviorStateRuntime(NativeBaseObjectMemory&, NativeEventDocumentRuntime&,
        NativeNpcManagerOwner&, NativeBehaviorHelperConfiguration&, INativeActorBehaviorResources&);
    NativeActorBehaviorStateRuntime(NativeBaseObjectMemory&, NativeEventDocumentRuntime&,
        NativeBehaviorStateGrid, NativeBehaviorHelperConfiguration&, INativeActorBehaviorResources&);
    std::uint32_t Restore(std::int32_t kind, std::uint32_t npc, std::uint32_t helper);
    std::uint32_t Save(std::int32_t kind, std::uint32_t npc, std::uint32_t helper);
    bool IsComplete(std::uint32_t npc);
private:
    class RestoreRuntime;
    class SaveRuntime;
    class AnimationSaveRuntime;
    void Validate(std::int32_t kind, std::uint32_t npc, std::uint32_t helper);
    NativeBaseObjectMemory& memory_;
    NativeEventDocumentRuntime& documents_;
    std::uint32_t Target(std::uint32_t actor, std::int32_t kind) const;
    std::uint32_t ManagerOffset(std::int32_t kind) const;
    NativeBehaviorStateGrid grid_;
    NativeBehaviorHelperConfiguration& configuration_;
    INativeActorBehaviorResources& resources_;
};
}
