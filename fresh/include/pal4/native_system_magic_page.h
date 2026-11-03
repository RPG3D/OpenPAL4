#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;class NativeInventorySession;class NativeBaseObjectMemory;
class NativeGameDatabase;class NativeSkillRecordCatalog;class NativePlayerSkillRuntime;
class NativePalGameIVTextureStorage;class NativeEventHandlerArray;class NativeCommonDialogLayout;
class NativeMagicWindowLayout;class NativeMagicWindowStuntAction;class NativeRoleSelector;
struct PlayerManagerObjectState;
struct NativeSystemMagicServices {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t)> tutorial;
    std::function<void(std::string_view,std::uint32_t)> sound;
    std::function<void(std::string_view)> learning_sound; // Original459780 full resource path, independent AudioSystem sample.
};
// Owns the persistent original Magic page, input/presentation, modal role choices
// and lazy learning/cast/Stunt controllers. Borrowed game/UI owners outlive Close.
class NativeSystemMagicPage final {
public:
    NativeSystemMagicPage(NativeCeguiUiSession&,const std::filesystem::path&,
        NativeInventorySession&,PlayerManagerObjectState&,const NativeGameDatabase&,
        const NativeSkillRecordCatalog&,NativeBaseObjectMemory&,NativePlayerSkillRuntime&,
        NativePalGameIVTextureStorage&,NativeEventHandlerArray&,std::uint32_t manager,
        NativeCommonDialogLayout&,NativeSystemMagicServices);
    ~NativeSystemMagicPage();
    NativeSystemMagicPage(const NativeSystemMagicPage&)=delete;
    NativeSystemMagicPage& operator=(const NativeSystemMagicPage&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    bool Visible()const noexcept;
    std::uint32_t FrameAddress()const noexcept;
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t,std::uint32_t);
    std::uint8_t Predicate(std::uint32_t,std::uint32_t,std::uint32_t);
    void DispatchCommand(std::string_view,std::uint32_t);
    NativeMagicWindowLayout& Layout();
    NativeRoleSelector& Selector();
    NativeMagicWindowStuntAction& Stunts();
    void RethrowError();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
