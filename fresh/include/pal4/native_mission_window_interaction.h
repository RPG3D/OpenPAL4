#pragma once
#include "pal4/mission_window_lifecycle.h"
#include <windows.h>
#include <functional>
#include <memory>

namespace pal4 {
class NativeEventHandlerArray;
// Borrows the layout/activity/update endpoint. Close before retiring them.
// Event subscriptions live with the DLL windows; Close disables their shared
// dispatch state and removes borrowed activity members before host destruction.
class NativeMissionWindowInteraction final {
public:
    NativeMissionWindowInteraction(HMODULE,MissionWindowState&,NativeEventHandlerArray&,
        std::function<void()> update_details,std::function<void(std::uint32_t)> story_click);
    ~NativeMissionWindowInteraction();
    NativeMissionWindowInteraction(const NativeMissionWindowInteraction&)=delete;
    NativeMissionWindowInteraction& operator=(const NativeMissionWindowInteraction&)=delete;
    bool OwnsActivity(std::uint32_t member) const noexcept;
    std::uint32_t DispatchActiveCallback(std::uint32_t member,std::uint32_t slot);
    std::uint8_t DispatchActivePredicate(std::uint32_t member,std::uint32_t slot,std::uint32_t event);
    void RethrowEventError();
    void Close();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
