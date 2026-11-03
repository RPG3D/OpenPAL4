#pragma once

#include "pal4/music_ui_state.h"
#include <functional>
#include <string>

namespace pal4 {
class NativeWorldMapStateOwner final {
public:
    std::uint32_t Construct();
    void Destroy() noexcept { image_ = 0u; }
    std::uint32_t Address() const noexcept;
private:
    std::uint32_t image_{};
};

struct NativeWorldMapStateServices {
    std::uint32_t manager{}, console{}, music{}, script_executor{};
    std::function<void(std::string_view, float, std::uint32_t)> play_music;
    std::function<void(double)> update_ui;
    std::function<std::uint32_t(double)> process_scripts;
    std::function<std::uint32_t(std::uint32_t)> render_ui;
};

// State11 reuses the recovered 5905F0/5906E0 behavior (legacy MusicUiState
// name), and renders UI only as its original vtable's 58F800 slot requires.
class NativeWorldMapStateRuntime final : private IMusicUiRuntime {
public:
    explicit NativeWorldMapStateRuntime(NativeWorldMapStateServices);
    bool Enter(std::uint32_t document);
    void Leave() noexcept {} // Original 5A92E0 return-one exit slot.
    std::uint32_t Update(float elapsed);
    std::uint32_t Render(std::uint32_t frame);
private:
    void WriteWord(std::uint32_t, std::uint32_t) override;
    std::uint32_t UIFrameManager() override;
    void SetUIFlag(std::uint32_t, std::uint32_t) override;
    std::uint32_t Console() override;
    bool AllocateTrack(std::string_view) override;
    void ReleaseTrack() override;
    std::uint32_t MusicPlayer() override;
    void FadeInMusic(std::uint32_t, std::string_view, double, std::uint32_t) override;
    void UpdateUI(std::uint32_t, double) override;
    std::uint32_t ScriptExecutor() override;
    std::uint32_t ProcessLog(std::uint32_t, double) override;
    NativeWorldMapStateServices services_;
    std::string track_;
};
} // namespace pal4
