#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCeguiUiSession;class NativeInventorySession;class NativeUiCollectionObjects;class NativeEffectSceneSession;
class NativePkFileManagerRuntime;class NativeEventHandlerArray;class NativePalGameIVTextureStorage;
class NativeGameDatabase;class NativePrescriptionRecordCatalog;class NativeBaseObjectMemory;
class NativePlayerScriptRefreshRuntime;class NativeCommonDialogLayout;struct PlayerManagerObjectState;
struct NativeSmithWindowServices {
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> event;
    std::function<void(std::uint32_t)> tutorial;
    std::function<void(std::string_view,std::uint32_t)> sound;
    std::function<bool()> smith_active;
    std::function<void(bool)> set_smith_active;
    std::function<std::uint32_t(std::string_view)> find_frame_root;
};
// Persistent original Smith page and all its controllers. Borrowed game/world/
// UI owners outlive Close; retire before releasing the UI collection/world.
class NativeSmithWindow final {
public:
    NativeSmithWindow(NativeCeguiUiSession&,const std::filesystem::path&,NativeInventorySession&,
        PlayerManagerObjectState&,NativeBaseObjectMemory&,NativePlayerScriptRefreshRuntime&,
        const NativeGameDatabase&,const NativePrescriptionRecordCatalog&,NativeUiCollectionObjects&,
        NativeEffectSceneSession&,NativePkFileManagerRuntime&,NativeEventHandlerArray&,std::uint32_t manager,
        NativePalGameIVTextureStorage&,NativeCommonDialogLayout&,NativeSmithWindowServices);
    ~NativeSmithWindow();
    NativeSmithWindow(const NativeSmithWindow&)=delete;
    NativeSmithWindow& operator=(const NativeSmithWindow&)=delete;
    void SetVisible(bool,std::uint32_t parent=0u);
    std::uint32_t FrameAddress()const noexcept;
    std::uint32_t RootAddress()const noexcept;
    std::uint32_t FrameWord(std::uint32_t offset)const;
    bool OwnsActivity(std::uint32_t)const noexcept;
    std::uint32_t Callback(std::uint32_t member,std::uint32_t slot);
    std::uint8_t Predicate(std::uint32_t member,std::uint32_t slot,std::uint32_t event);
    void RethrowEventError();
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
