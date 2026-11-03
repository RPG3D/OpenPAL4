#pragma once

#include "pal4/native_base_object_member_runtime.h"
#include "pal4/native_borrowed_actor_binding.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_game_manager_component_runtime.h"
#include "pal4/native_monster_behavior_tree_runtime.h"
#include "pal4/native_script_object_runtime.h"

#include <span>
#include <functional>
#include <memory>
#include <string_view>

namespace pal4 {

class BinaryXmlConfig;
class IBaseObjectActivityRuntime;
class INativeBehaviorHelpers;
class NativeBehaviorTasks;

using NativeNpcAnimationClip = NativeBorrowedAnimationClip;

// Owns the component image and action records; actor clumps and clips are borrowed.
class NativeNpcObjectOwner final {
public:
    NativeNpcObjectOwner(NativeBaseObjectMemory& memory,
                         IRwDictSchemaRuntime& schema,
                         IGameObjectEmbeddedAnimationRuntime& animation);
    ~NativeNpcObjectOwner();
    NativeNpcObjectOwner(const NativeNpcObjectOwner&) = delete;
    NativeNpcObjectOwner& operator=(const NativeNpcObjectOwner&) = delete;

    [[nodiscard]] std::uint32_t Construct();
    void BindBorrowedActor(std::uint32_t main_clump,
                           std::uint32_t shadow_clump);
    void BindBorrowedAnimations(std::string_view base_path,
                                std::span<const NativeNpcAnimationClip> clips);
    [[nodiscard]] bool SelectBorrowedAction(std::string_view action);
    void SetTransform(const ResourceVec3& position,
                      const ResourceVec3& rotation,
                      IGameResourceManagerRuntime& runtime);
    void SetStateFlag(std::uint32_t value);
    void BindManagerGrid(std::uint32_t grid_member);
    void UnbindManagerGrid();
    void SetupBehavior(std::uint32_t npc_record,
                       const BinaryXmlConfig& behavior,
                       IGameResourceManagerRuntime& transforms,
                       bool clear_previous = false);
    void PauseBehavior(IGameResourceManagerRuntime& transforms);
    void BindBehaviorHelpers(std::shared_ptr<INativeBehaviorHelpers> helpers);
    void UnbindBehavior();
    // Original object virtual+44 (4393E0), using explicit live resource services.
    [[nodiscard]] std::uint32_t CleanupActivity(
        IBaseObjectActivityRuntime& activity, IGameResourceManagerRuntime& transforms);
    void UnbindBorrowedActor();
    [[nodiscard]] std::uint32_t ProcessWorldFrame(
        float elapsed, std::uint32_t mode,
        const std::function<std::uint32_t(float, std::uint32_t)>& motion);
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept { return object_; }
    [[nodiscard]] bool ActorBound() const noexcept { return actor_bound_; }
    // Use the constructing runtime for local ownership; its allocation domain shares the tree nil.
    [[nodiscard]] NativeEventDocumentRuntime& BehaviorDocuments() noexcept { return document_; }

private:
    NativeBaseObjectMemory& memory_;
    IGameObjectEmbeddedAnimationRuntime& animation_;
    NativeBaseObjectMemberRuntime base_;
    NativeEventDocumentRuntime document_;
    NativeScriptObjectRuntime script_;
    NativeMonsterBehaviorTreeRuntime tree_;
    NativeGameManagerComponentRuntime components_;
    std::unique_ptr<NativeBehaviorTasks> behavior_tasks_;
    std::uint32_t object_ = 0u;
    bool actor_bound_ = false;
};

}  // namespace pal4
