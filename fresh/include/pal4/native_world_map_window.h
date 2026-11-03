#pragma once
#include "pal4/world_map_buttons.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class NativeEventHandlerArray;
class NativeGameTimerQueue;
class NativeCommonDialogLayout;
// Borrows real SDK module/context, common-dialog and UI timer owners. Script
// methods carry actual interpreter handles; the window never invents modules.
struct NativeWorldMapServices {
    std::function<std::string()> current_arena;
    std::function<void()> set_default_cursor;
    std::function<std::uint32_t()> script_owner;
    std::function<std::string_view(std::uint32_t)> function_name;
    std::function<std::string_view(std::uint32_t)> full_path;
    std::function<std::uint32_t(std::string_view object,std::string_view entry)> create_interpreter;
    std::function<bool(std::uint32_t)> add_script_module;
    std::function<void(std::uint32_t)> execute_immediate;
    std::function<void(std::uint32_t)> push_prepared_module;
    std::function<void(std::uint32_t)> release_interpreter;
    std::function<void()> cleanup_scripts;
    std::function<NativeCommonDialogLayout&()> acquire_common_dialog;
    std::function<void(std::string_view,std::uint32_t)> play_sound;
};
class NativeWorldMapWindow final {
public:
    NativeWorldMapWindow(NativeCeguiUiSession&,const std::filesystem::path&,
        const NativeGameDatabase&,NativeEventHandlerArray&,NativeGameTimerQueue&,
        std::uint32_t manager,NativeWorldMapServices);
    ~NativeWorldMapWindow();
    NativeWorldMapWindow(const NativeWorldMapWindow&)=delete;
    NativeWorldMapWindow& operator=(const NativeWorldMapWindow&)=delete;
    void Show(std::uint32_t parent=0u);
    void Hide();
    bool Visible()const noexcept;
    std::uint32_t FrameAddress()const noexcept;
    std::uint32_t RootAddress()const noexcept;
    std::uint32_t Word(std::uint32_t offset)const;
    IWorldMapButtonStateRuntime& Buttons() noexcept;
    IWorldMapExternalButtonRuntime& ExternalButtons() noexcept;
    void AdvanceFlight(std::string_view destination);
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t,std::uint32_t);
    std::uint8_t Predicate(std::uint32_t,std::uint32_t,std::uint32_t);
    void RethrowError();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
