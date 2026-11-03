#pragma once
#include "pal4/mission_window_lifecycle.h"
#include <windows.h>
#include <memory>

namespace pal4 {
class NativeGameDatabase;
class NativePalGameIVTextureStorage;
// Binds the shared mission-list algorithm to original DLL windows/items and the
// real database/Story owner. The layout and DLLs must outlive this binding.
// Clear/retire this owner before destroying its borrowed List control.
class NativeMissionWindowList final {
public:
    NativeMissionWindowList(HMODULE cegui,HMODULE look,MissionWindowState&,
        const NativeGameDatabase&,NativePalGameIVTextureStorage&);
    ~NativeMissionWindowList();
    NativeMissionWindowList(const NativeMissionWindowList&) = delete;
    NativeMissionWindowList& operator=(const NativeMissionWindowList&) = delete;
    void Populate();
    // Requires the lifecycle's single configured ListStory column.
    void PopulateChapters();
    void Clear();
    std::size_t OwnedItemCount() const noexcept;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
