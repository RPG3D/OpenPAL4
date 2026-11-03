#pragma once
#include "pal4/save_window_lifecycle.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>
namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class DialogManagerState;
class NativeEventHandlerArray;
class NativeCommonDialogLayout;
struct NativeSaveWindowServices {
    std::function<std::pair<std::uint32_t,std::uint32_t>()> current_state;
    std::function<void()> capture_raster;
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::string_view,std::uint32_t)> sound;
    std::function<void()> save_settings;
    std::function<void(std::int32_t)> save_slot;
};
// Owns original layout, rows and callbacks; borrowed UI/globals/activity/dialog
// and game services must outlive Close. No original EXE vtable is invoked.
class NativeSaveWindow final {
public:
    NativeSaveWindow(NativeCeguiUiSession&,const std::filesystem::path& asset_root,
        const std::filesystem::path& archive_root,const NativeGameDatabase&,DialogManagerState&,
        NativeEventHandlerArray&,NativeCommonDialogLayout&,std::uint32_t manager,NativeSaveWindowServices);
    ~NativeSaveWindow();
    NativeSaveWindow(const NativeSaveWindow&)=delete;
    NativeSaveWindow& operator=(const NativeSaveWindow&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    bool Visible() const noexcept;
    const SaveWindowState& State()const noexcept;
    std::uint32_t InputAddress()const noexcept;
    std::uint32_t Callback(std::uint32_t slot);
    std::uint8_t Predicate(std::uint32_t slot,std::uint32_t event);
    void RethrowError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
