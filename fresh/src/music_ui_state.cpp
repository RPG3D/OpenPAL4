#include "pal4/music_ui_state.h"

namespace pal4 {

std::uint32_t MusicUiState::Construct(
    std::uint32_t object, IMusicUiRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    return object;
}

std::uint32_t MusicUiState::Enter(
    std::uint32_t, std::uint32_t, IMusicUiRuntime& runtime) {
    runtime.SetUIFlag(runtime.UIFrameManager(), 1u);
    runtime.WriteWord(runtime.Console(), 0u);
    const bool allocated = runtime.AllocateTrack("P01-1.mp3");
    try {
        runtime.FadeInMusic(runtime.MusicPlayer(),
                            allocated ? "P01-1.mp3" : "", 3.0, 0u);
    } catch (...) {
        if (allocated) runtime.ReleaseTrack();
        throw;
    }
    if (allocated) runtime.ReleaseTrack();
    return 1u;
}

std::uint32_t MusicUiState::Update(
    float delta, std::uint32_t, std::uint32_t,
    IMusicUiRuntime& runtime) {
    runtime.UpdateUI(runtime.UIFrameManager(), static_cast<double>(delta));
    return runtime.ProcessLog(runtime.ScriptExecutor() + 84u,
                              static_cast<double>(delta));
}

std::uint32_t MusicUiState::GetTypeCode() {
    return 11u;
}

}  // namespace pal4
