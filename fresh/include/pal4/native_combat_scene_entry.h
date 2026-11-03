#pragma once
#include "pal4/native_combat_scene_lifecycle.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCombatStateOwner;struct NativeCombatSystemImage;class GameRenderState;
class DialogManagerState;class NativeCameraManagerOwner;class NativeCeguiUiSession;
class PalConsoleState;class NativeCommonDialogLayout;class NativeGameDatabase;
struct NativeCombatSceneEntryServices {
    std::function<bool(std::uint32_t document,std::uint32_t mode)> start;
    std::function<void()> close_ui;
    std::function<void(std::string_view,std::uint32_t)> sound;
};
// Actual58ADB0/58B270/58B2D0 over one registered owner. Callbacks borrow the
// owner until Leave/Close. This alone does not bind render/update/state exchange.
class NativeCombatSceneEntry final : public INativeCombatSceneLifecycle {
public:
    NativeCombatSceneEntry(NativeCombatStateOwner&,NativeCombatSystemImage&,GameRenderState&,
        DialogManagerState&,NativeCameraManagerOwner&,NativeCeguiUiSession&,PalConsoleState&,
        NativeCommonDialogLayout&,const NativeGameDatabase&,std::uint32_t frame_manager,
        const std::filesystem::path& root,NativeCombatSceneEntryServices);
    ~NativeCombatSceneEntry() override;
    std::uint32_t Address() const override;
    bool Enter(std::uint32_t document) override;
    void Leave() override;
    void Close();
    bool Entered()const noexcept;
    bool WaitingChoice()const noexcept;
    std::size_t Starts()const noexcept;
    std::uint32_t LastMode()const noexcept;
private:
    class Impl;std::shared_ptr<Impl> impl_;
};
}
