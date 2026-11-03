#pragma once
#include "pal4/native_animation_event_queue_runtime.h"
#include <functional>

namespace pal4 {
class IUiTextImageOverlayRuntime;

// Native slot0/4/8/12 dispatch for the six verified PAL4 animation vtables.
// Borrows the overlay runtime and required object deallocator. Close all queues
// before either dependency dies. The overlay owns text, not the object itself.
class NativeUiTextImageActivities final : public IAnimationEventQueueActivityRuntime {
public:
    NativeUiTextImageActivities(IUiTextImageOverlayRuntime&,
        std::function<void(std::uint32_t)> free_object);
    void Validate(std::uint32_t animation) const;
    void DeleteAnimation(std::uint32_t) override;
    void AdvanceAnimation(std::uint32_t,float) override;
    void CallAnimationEvent(std::uint32_t,std::uint32_t) override;
    std::uint32_t AnimationFinished(std::uint32_t) override;
private:
    IUiTextImageOverlayRuntime& overlay_;
    std::function<void(std::uint32_t)> free_object_;
};
}
