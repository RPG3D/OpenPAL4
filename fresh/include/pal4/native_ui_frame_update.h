#pragma once

#include "pal4/native_animation_event_queue_runtime.h"
#include "pal4/ui_frame_manager_singleton.h"
#include <functional>
#include <map>
#include <vector>

namespace pal4 {
class NativeCeguiUiSession;
class NativeGameTimerQueue;
struct CollectionState;
class ICollectionUpdateRuntime;

// Borrows the frame-manager image and engine/UI owners. Owns its embedded
// animation queue nodes. A collection borrows its state and update services;
// unbind before destroying either. Unsupported general frames fail explicitly
// instead of invoking original EXE vtables on host objects.
class NativeUiFrameUpdate final : private IUiFrameManagerUpdateRuntime {
public:
    NativeUiFrameUpdate(std::uint32_t manager, NativeCeguiUiSession& ui,
        NativeGameTimerQueue& timers, IAnimationEventQueueActivityRuntime& animations);
    ~NativeUiFrameUpdate();
    NativeUiFrameUpdate(const NativeUiFrameUpdate&) = delete;
    NativeUiFrameUpdate& operator=(const NativeUiFrameUpdate&) = delete;
    // Publish the actual CEGUI owner after System creation, before input can
    // arrive; cold menu input can precede the first timed frame.
    void BindSystem();
    [[nodiscard]] std::uint32_t Update(double elapsed);
    std::uint32_t Render(std::uint32_t frame);
    void AddAnimation(std::uint32_t animation);
    void BindCollection(CollectionState& collection, ICollectionUpdateRuntime& runtime);
    void UnbindCollection();
    // Typed implementations of the original visible-frame list at18C/190.
    // Owners unbind before retirement; callbacks cannot mutate this registry.
    void BindFrame(std::uint32_t,std::function<bool()> visible,std::function<void(double)> update);
    void UnbindFrame(std::uint32_t);
    // Original manager+1D0 is updated once before the ordinary frame list.
    void BindSpecialFrame(std::uint32_t, std::function<bool()>, std::function<void(double)>);
    void UnbindSpecialFrame(std::uint32_t);
    void Release();
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }
private:
    std::uint32_t ReadWord(std::uint32_t address) override;
    bool IsVisible(std::uint32_t frame) override;
    void InjectTimePulse(std::uint32_t system, float elapsed) override;
    void RotateGameObjects(std::uint32_t collection, double elapsed) override;
    void UpdateJumpHint(std::uint32_t frame, float elapsed) override;
    void UpdateFrame(std::uint32_t frame, double elapsed) override;
    void UpdateTimedEffects(std::uint32_t effects, float elapsed) override;
    std::uint32_t UpdateAnimations(std::uint32_t list, float elapsed) override;
    void StoreWord(std::uint32_t offset, std::uint32_t value);
    void CheckFrames();
    void PublishFrames();

    std::uint32_t manager_;
    NativeCeguiUiSession& ui_;
    NativeGameTimerQueue& timers_;
    NativeBaseObjectMemory memory_;
    NativeAnimationEventQueueRuntime animations_;
    CollectionState* collection_ = nullptr;
    ICollectionUpdateRuntime* collection_runtime_ = nullptr;
    bool constructed_ = false;
    bool dispatching_ = false;
    struct Frame {std::function<bool()> visible;std::function<void(double)> update;};
    std::map<std::uint32_t,Frame> frame_bindings_;
    std::vector<std::uint32_t> frames_;
};
}  // namespace pal4
