#pragma once
#include "pal4/mission_window_lifecycle.h"
#include <memory>

namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
// Borrows the task layout/UI; retire before either. Reuses the existing hover
// lookup and quest-details algorithm, including its null-selection behavior.
class NativeMissionWindowDetails final {
public:
    NativeMissionWindowDetails(NativeCeguiUiSession&,MissionWindowState&,const NativeGameDatabase&);
    ~NativeMissionWindowDetails();
    NativeMissionWindowDetails(const NativeMissionWindowDetails&)=delete;
    NativeMissionWindowDetails& operator=(const NativeMissionWindowDetails&)=delete;
    void UpdateHovered();
    void HandleStoryClick(std::uint32_t event);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
