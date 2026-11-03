#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;class NativeSmithWindowLayout;class NativeSmithWindowLists;
class NativeSmithWindowTabs;class NativeSmithWindowPreview;class NativeEventHandlerArray;
class NativePalGameIVTextureStorage;
struct NativeSmithVisibilityServices {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<bool()> smith_active;
    std::function<void(bool)> set_smith_active;
    // Returns a live DLL root for the named registered frame, or zero if absent.
    std::function<std::uint32_t(std::string_view)> find_frame_root;
};
// Original 497420 lifecycle, borrowing existing page/model/list/activity owners.
// Includes original navigation activity, vertical keys, shortcuts and ESC events.
// Close before tabs/preview/lists/layout; the application supplies event routing.
class NativeSmithWindowVisibility final {
public:
    NativeSmithWindowVisibility(NativeCeguiUiSession&,NativeSmithWindowLayout&,
        NativeSmithWindowLists&,NativeSmithWindowTabs&,NativeSmithWindowPreview&,
        NativeEventHandlerArray&,std::uint32_t manager,NativePalGameIVTextureStorage&,
        NativeSmithVisibilityServices);
    ~NativeSmithWindowVisibility();
    NativeSmithWindowVisibility(const NativeSmithWindowVisibility&)=delete;
    NativeSmithWindowVisibility& operator=(const NativeSmithWindowVisibility&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    bool IsAttached()const;
    std::uint32_t DispatchActivity(std::uint32_t member,std::uint32_t slot);
    std::uint8_t DispatchInput(std::uint32_t member,std::uint32_t slot,std::uint32_t event);
    bool FromDllNavigation(std::uint32_t handler,std::uint32_t event);
    void Close();
private:
    class Runtime;std::shared_ptr<Runtime> runtime_;
};
}
