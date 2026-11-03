#pragma once
#include "pal4/logo_state.h"
#include "pal4/effect_system_object_f_queue.h"
#include <filesystem>
#include <functional>

namespace pal4 {
class NativeCameraManagerOwner;
class NativeUiCollectionScene;
class NativeEffectSceneSession;
class NativeDialogSettingsSnapshot;
class NativeDialogSettingsFileWriter;
class BackgroundMusicPlayer;
class NativeBackgroundMusicRuntime;

struct NativeLogoStateServices {
    std::function<void(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t)> event;
    std::function<void(std::uint32_t, bool)> edges;
};

// Owns the Logo state words; borrows all running services. Leave before
// destroying or unregistering it, and release the UI collection after Leave.
// Frame update/render and state-registry dispatch remain separate owners.
class NativeLogoStateLifecycle final : private ILogoStateLifecycleRuntime {
public:
    NativeLogoStateLifecycle(std::uint32_t manager, std::filesystem::path root,
        NativeCameraManagerOwner&, NativeUiCollectionScene&, NativeEffectSceneSession&,
        NativeDialogSettingsSnapshot&, NativeDialogSettingsFileWriter&,
        BackgroundMusicPlayer&, NativeBackgroundMusicRuntime&, NativeLogoStateServices);
    NativeLogoStateLifecycle(const NativeLogoStateLifecycle&) = delete;
    NativeLogoStateLifecycle& operator=(const NativeLogoStateLifecycle&) = delete;
    std::uint32_t Address() const;
    std::uint32_t Enter();
    std::uint32_t Leave();
    bool Entered() const noexcept { return entered_; }
    EffectSystemHandleWords Handle() const { return {words_[1], words_[2]}; }
    std::uint32_t CountdownBits() const { return words_[3]; }
private:
    friend class NativeLogoStateUpdate;
    friend class NativeLogoStateRenderer;
    void CheckManager(std::uint32_t) const;
    EffectSystemHandleWords ReadHandle(std::uint32_t) const;
    std::uint32_t ReadWord(std::uint32_t) override;
    void WriteWord(std::uint32_t, std::uint32_t) override;
    std::uint32_t GlobalData() override;
    void SaveDialogSettings(std::uint32_t) override;
    std::uint32_t UIFrameManager() override;
    void SetUIFlag(std::uint32_t, std::uint32_t) override;
    std::uint32_t MusicPlayer() override;
    void FadeInMusic(std::uint32_t, std::string_view, double, std::uint32_t) override;
    void AddUIElement(std::uint32_t, std::string_view) override;
    void SetupUICamera(std::uint32_t, std::uint32_t) override;
    void SendUIEvent(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) override;
    std::uint32_t CameraManager() override;
    std::uint32_t UICameraName() override;
    void ActivateCamera(std::uint32_t, std::uint32_t) override;
    void ToggleWindows(std::uint32_t, bool) override;
    bool EffectNeedsUpdate(std::uint32_t) override;
    void HandleEffect(std::uint32_t, std::uint32_t) override;
    LogoEffectHandle CreateEffect(std::uint32_t, std::string_view, const std::array<float, 3>&,
        std::uint32_t, std::uint32_t) override;
    void ResetTemporaryEffectHandle() override;
    void SetEffectProperties(std::uint32_t, const std::array<float, 3>&, std::uint8_t) override;
    void CleanupEffects(std::uint32_t) override;
    std::array<std::uint32_t, 4> words_{0u, 0xffffffffu, 0u, 0u};
    EffectSystemHandleWords temporary_{};
    std::uint32_t manager_;
    std::filesystem::path root_;
    NativeCameraManagerOwner& cameras_;
    NativeUiCollectionScene& scene_;
    NativeEffectSceneSession& effects_;
    NativeDialogSettingsSnapshot& settings_;
    NativeDialogSettingsFileWriter& writer_;
    BackgroundMusicPlayer& music_;
    NativeBackgroundMusicRuntime& audio_;
    NativeLogoStateServices services_;
    bool entered_ = false;
};
}
