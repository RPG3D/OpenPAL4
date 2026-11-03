#pragma once
#include "pal4/mission_window_lifecycle.h"
#include <filesystem>
#include <functional>
#include <memory>

namespace pal4 {
class NativeCeguiUiSession;
class NativeGameDatabase;
class NativePalGameIVTextureStorage;
class NativeEventHandlerArray;
struct NativeMissionWindowServices {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t)> tutorial;
};
// Owns the layout, navigation, list items and typed activity dispatch. Borrowed
// UI/database/game/activity and service endpoints must outlive Close.
class NativeMissionWindow final {
public:
    NativeMissionWindow(NativeCeguiUiSession&,const std::filesystem::path&,
        const NativeGameDatabase&,NativePalGameIVTextureStorage&,
        NativeEventHandlerArray&,std::uint32_t manager,NativeMissionWindowServices);
    ~NativeMissionWindow();
    NativeMissionWindow(const NativeMissionWindow&)=delete;
    NativeMissionWindow& operator=(const NativeMissionWindow&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    std::uint32_t FrameAddress() const noexcept;
    const MissionWindowState& State() const noexcept;
    bool OwnsActivity(std::uint32_t) const noexcept;
    std::uint32_t DispatchActiveCallback(std::uint32_t,std::uint32_t);
    std::uint8_t DispatchActivePredicate(std::uint32_t,std::uint32_t,std::uint32_t);
    void RethrowEventError();
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
