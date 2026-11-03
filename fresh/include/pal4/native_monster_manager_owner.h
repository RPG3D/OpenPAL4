#pragma once

#include "pal4/monster_info_manager.h"
#include "pal4/native_base_object_scene_frame.h"
#include "pal4/monster_manager_access.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_monster_info_reader.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace pal4 {

class IMonsterPursuitCollisionGateRuntime;
class NativeMonsterObjectOwner;
class NativeActorAnimationStateBinding;
struct GridManagerMembersState;
class IBaseObjectMaterialRuntime;
class IBaseObjectAttachedEffectRuntime;

// Host dispatch for helper vtable +0xA4/+0xAC. A successful registration
// retains this service until release; helper storage remains its responsibility.
// Release is nonthrowing and must not reenter or mutate the manager collection.
// The service must retain all allocator/storage dependencies used by Release.
class INativeMonsterGridElementRuntime {
public:
    virtual ~INativeMonsterGridElementRuntime() = default;
    virtual void Update(std::uint32_t element, GridManagerMembersState& members,
                         std::uint32_t argument) = 0;
    virtual void Release(std::uint32_t element) noexcept = 0;
};

class NativeMonsterManagerOwner final {
public:
    using PursuitReset = std::function<std::uint32_t()>;
    using PursuitEntry = std::function<std::uint32_t(std::uint32_t)>;
    using AnimationSelector = std::function<bool(std::string_view)>;
    using FrameRender = std::function<void(std::uint32_t, NativeBaseObjectSceneFrameResult&)>;
    using ActivityCleanup = std::function<void(std::uint32_t,IBaseObjectMaterialRuntime&)>;
    NativeMonsterManagerOwner();
    void BindAttachedEffectRuntime(IBaseObjectAttachedEffectRuntime* runtime) noexcept { attached_=runtime; }
    IBaseObjectAttachedEffectRuntime* AttachedEffectRuntime() const noexcept { return attached_; }
    ~NativeMonsterManagerOwner();
    NativeMonsterManagerOwner(const NativeMonsterManagerOwner&) = delete;
    NativeMonsterManagerOwner& operator=(const NativeMonsterManagerOwner&) = delete;

    [[nodiscard]] std::uint32_t Construct();
    [[nodiscard]] bool LoadInfo(std::span<const NativeMonsterInfoRecord> records);
    [[nodiscard]] bool InitializeScene(
        std::span<const NativeMonsterInfoRecord> records,
        const std::function<void(const NativeMonsterInfoRecord&)>& create = {});
    [[nodiscard]] std::uint32_t SetPursuitTargetAll(std::uint32_t actor);
    [[nodiscard]] std::uint32_t ClearStopPursuitAll();
    [[nodiscard]] std::uint32_t StopPursuitAll(
        float elapsed, IMonsterPursuitCollisionGateRuntime* pursuit = nullptr);
    [[nodiscard]] std::uint32_t ProcessCollisions(
        float delta, IMonsterPursuitCollisionGateRuntime* pursuit = nullptr);
    [[nodiscard]] std::uint32_t ResetOwnedPursuit(std::uint32_t object);
    [[nodiscard]] std::uint32_t EnterOwnedPursuit(
        std::uint32_t object, std::uint32_t target);
    [[nodiscard]] bool SelectOwnedAnimation(
        std::uint32_t object, std::string_view action);
    [[nodiscard]] NativeMonsterObjectOwner& OwnedComponent(std::uint32_t object);
    [[nodiscard]] NativeActorAnimationStateBinding& AnimationBinding(std::uint32_t object);
    [[nodiscard]] std::uint32_t ForEachObject(
        const std::function<std::uint32_t(std::uint32_t)>& callback);
    [[nodiscard]] NativeBaseObjectSceneFrameResult RenderFrame(std::uint32_t camera);
    // IDA437CF0: grid first, then active objects' +0x38 world updates.
    // Dispatch may change pursuit/animation and visit the stable tree, but may
    // not reload, register or destroy entities during this traversal.
    [[nodiscard]] std::uint32_t ProcessWorldObjects(float elapsed,
        std::uint32_t mode,
        const std::function<void(std::uint32_t, float, std::uint32_t)>& dispatch);
    void VisitObjects(const std::function<bool(std::uint32_t)>& visitor);
    [[nodiscard]] std::uint32_t FindInfo(std::string_view name);
    [[nodiscard]] bool RegisterConstructedObject(std::string_view name,
                                                 std::uint32_t object);
    template <typename T>
    [[nodiscard]] bool RegisterOwnedObject(std::string_view name,
                                            std::uint32_t object,
                                            std::unique_ptr<T> owner,
                                            PursuitReset reset = {},
                                            PursuitEntry enter = {},
                                            AnimationSelector select = {},
                                            NativeMonsterObjectOwner* component = nullptr,
                                            ActivityCleanup cleanup = {},
                                            FrameRender render = {},
                                            NativeActorAnimationStateBinding* animation = nullptr) {
        if (!owner)
            throw std::invalid_argument("owned monster registration requires an owner");
        OwnedObject erased(owner.release(), [](void* value) {
            delete static_cast<T*>(value);
        });
        return RegisterOwnedObjectImpl(name, object, std::move(erased),
                                       std::move(reset), std::move(enter),
                                       std::move(select), component,std::move(cleanup),std::move(render),animation);
    }
    [[nodiscard]] std::uint32_t FindObject(std::string_view name);
    [[nodiscard]] std::uint32_t ObjectCount() const;
    [[nodiscard]] const BinaryXmlConfig* InfoDocument(
        std::uint32_t address) const;
    [[nodiscard]] std::uint32_t InfoCount() const;
    [[nodiscard]] std::uint32_t GridAddress() const noexcept;
    [[nodiscard]] std::uint32_t GridWrapperAddress() const noexcept;
    [[nodiscard]] std::vector<std::uint32_t> GridObstacleCandidates() const;
    // Transfer helper lifetime to the collection only on success. Collection
    // buffers use manager memory; Release must destroy/free the helper using
    // its original allocator before the borrowed spatial grid is reset.
    [[nodiscard]] bool AddGridElement(std::uint32_t element,
        std::shared_ptr<INativeMonsterGridElementRuntime> runtime);
    [[nodiscard]] bool RemoveGridElement(std::uint32_t element);
    [[nodiscard]] std::uint8_t UpdateGridElements(std::uint32_t argument);
    [[nodiscard]] std::uint32_t GridElementCount() const noexcept;
    [[nodiscard]] std::uint32_t Address() const noexcept { return object_; }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept;
    void Destroy();
    // IDA437B60: invokes the supplied +44 service before each owned entity's
    // deletion, clears scene records/grid and retains the manager identity.
    void CleanupScene(const std::function<void(std::uint32_t)>& notify);
    void CleanupOwnedActivity(std::uint32_t object,std::uint32_t game,
        IBaseObjectMaterialRuntime& materials);

private:
    using OwnedObject = std::unique_ptr<void, void (*)(void*)>;
    struct OwnedEntry {
        OwnedObject owner;
        std::uint32_t object;
        PursuitReset reset;
        PursuitEntry enter;
        AnimationSelector select;
        NativeMonsterObjectOwner* component;
        ActivityCleanup cleanup;
        FrameRender render;
        NativeActorAnimationStateBinding* animation;
    };
    [[nodiscard]] bool RegisterOwnedObjectImpl(std::string_view name,
                                                std::uint32_t object,
                                                OwnedObject owner,
                                                PursuitReset reset,
                                                PursuitEntry enter,
                                                AnimationSelector select,
                                                NativeMonsterObjectOwner* component,
                                                ActivityCleanup cleanup, FrameRender render,
                                                NativeActorAnimationStateBinding* animation);
    class InfoRuntime;
    class GridRuntime;
    class TreeRuntime;
    class ObjectRuntime;
    class SceneRuntime;
    class CleanupRuntime;

    NativeBaseObjectMemory memory_;
    std::unique_ptr<InfoRuntime> info_runtime_;
    std::unique_ptr<GridRuntime> grid_runtime_;
    std::unique_ptr<TreeRuntime> tree_runtime_;
    std::unique_ptr<ObjectRuntime> object_runtime_;
    std::unique_ptr<MonsterInfoManager> info_manager_;
    MonsterInfoSharedTree info_tree_{};
    MonsterManagerTreeGlobals object_tree_{};
    std::vector<OwnedEntry> owned_objects_;
    std::uint32_t object_ = 0u;
    bool rendering_ = false;
    bool updating_world_ = false;
    std::uint32_t traversal_depth_ = 0u;
    void RequireFrameIdle() const;
    void RequireCollectionIdle() const;
    bool updating_grid_ = false;
    bool cleaning_scene_ = false;
    IBaseObjectAttachedEffectRuntime* attached_{};
};

}  // namespace pal4
