#pragma once

#include <cstdint>
#include "pal4/minimap_scene_ui_visibility.h"

namespace pal4 {

class NativeGameTimerQueue;

class NativeSceneHudStatsTimerRuntime final : public IMinimapSceneUiStatsTimerRuntime {
public:
    explicit NativeSceneHudStatsTimerRuntime(NativeGameTimerQueue& queue) : queue_(queue) {}
    void RemoveStatsTimer(bool exception_message) override;
    void AddStatsTimer(std::uint32_t owner) override;
private:
    NativeGameTimerQueue& queue_;
};

// Timer phase of 0x478A00. The host dispatches 0x479470 for this owner.
// The queue must outlive the registered owner; hide removes its callback.
void ConfigureNativeSceneHudStatsTimer(NativeGameTimerQueue& queue,
                                      std::uint32_t owner, bool enabled);

}  // namespace pal4
