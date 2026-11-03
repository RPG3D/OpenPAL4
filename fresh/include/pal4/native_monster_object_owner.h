#pragma once

#include "pal4/native_borrowed_actor_binding.h"
#include "pal4/native_monster_behavior_runtime.h"

#include <functional>
#include "pal4/native_arena_cache_save_section.h"

namespace pal4 {

class BinaryXmlConfig;
class IBaseObjectActivityRuntime;

// Owns one constructed monster image; clumps and animation clips are borrowed.
class NativeMonsterObjectOwner final {
public:
    NativeMonsterObjectOwner(NativeBaseObjectMemory& memory,
                             IRwDictSchemaRuntime& schema,
                             IGameObjectEmbeddedAnimationRuntime& animation);
    ~NativeMonsterObjectOwner();
    NativeMonsterObjectOwner(const NativeMonsterObjectOwner&) = delete;
    NativeMonsterObjectOwner& operator=(const NativeMonsterObjectOwner&) = delete;

    [[nodiscard]] std::uint32_t Construct();
    void BindBorrowedActor(std::uint32_t main_clump,
                           std::uint32_t shadow_clump);
    void BindBorrowedAnimations(
        std::string_view base_path,
        std::span<const NativeBorrowedAnimationClip> clips);
    [[nodiscard]] bool SelectBorrowedAction(std::string_view action);
    // The selector switches both the borrowed record and the native HAnim clip.
    [[nodiscard]] std::uint32_t ResetPursuit(
        IGameResourceManagerRuntime& transforms,
        const std::function<bool(std::string_view)>& select_animation);
    [[nodiscard]] std::uint32_t EnterPursuit(
        std::uint32_t target,
        const std::function<bool(std::string_view)>& select_animation);
    void ApplyComponentField(std::uint32_t slot, std::uint32_t field,
                              std::uint32_t argument,
                              IGameResourceManagerRuntime& transforms);
    void SetTransform(const ResourceVec3& position,
                      const ResourceVec3& rotation,
                      IGameResourceManagerRuntime& runtime);
    void SetStateFlag(std::uint32_t value);
    [[nodiscard]] std::optional<NativeArenaCacheChild> CaptureStorageDocument(
        const BinaryXmlConfig& storage_config);
    [[nodiscard]] std::uint32_t RestoreStorageDocument(
        const BinaryXmlConfig& snapshot, const BinaryXmlConfig& storage_config);
    void BindManagerGrid(std::uint32_t grid_member);
    void InitializeComponent(std::uint32_t info_record,
                              const BinaryXmlConfig& behavior);
    std::uint32_t CleanupBehaviorBuffers(bool notify,
        IGameResourceManagerRuntime& transforms);
    std::uint32_t ReplaceBehaviorComponent(std::uint32_t source, bool dispatch_fields,
        IGameResourceManagerRuntime& transforms);
    void UnbindManagerGrid();
    void UnbindBorrowedActor();
    [[nodiscard]] std::uint32_t CleanupActivity(IBaseObjectActivityRuntime& activity,
        IGameResourceManagerRuntime& transforms);
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept { return object_; }
    [[nodiscard]] bool ActorBound() const noexcept { return actor_bound_; }
    [[nodiscard]] NativeMonsterBehaviorRuntime& Runtime() noexcept {
        return runtime_;
    }

private:
    NativeBaseObjectMemory& memory_;
    IGameObjectEmbeddedAnimationRuntime& animation_;
    NativeMonsterBehaviorRuntime runtime_;
    std::uint32_t object_ = 0u;
    bool actor_bound_ = false;
};

}  // namespace pal4
