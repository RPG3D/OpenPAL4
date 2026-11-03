#include "pal4/movie_play_window_playback.h"

namespace pal4 {

std::uint32_t MoviePlayWindowPlayback::Play(
    const MoviePlayWindowState& state,
    IMoviePlayWindowPlaybackRuntime& runtime) {
    auto player = runtime.BinkPlayer();
    if (!runtime.PlayerSoundInitialized(player)) {
        auto audio = runtime.AudioManager();
        if (!runtime.AudioInitialized(audio)) {
            audio = runtime.AudioManager();
            runtime.InitializeAudio(audio, 0u);
        }
        audio = runtime.AudioManager();
        const auto backend = runtime.AudioBackend(audio);
        player = runtime.BinkPlayer();
        runtime.SetSoundSystem(player, 1u, backend);
    }
    player = runtime.BinkPlayer();
    runtime.SetPlayerCallback(player, kPlayerCallbackOffset, 0u);
    player = runtime.BinkPlayer();
    runtime.CloseVideo(player);
    player = runtime.BinkPlayer();
    runtime.OpenVideo(player, state.Read(0x38u), 1u);
    player = runtime.BinkPlayer();
    runtime.SetPlayerCallback(player, kPlayerCallbackOffset,
                              kProcessMovieEvent);
    return player;
}

std::uint32_t MoviePlayWindowPlayback::TogglePause(
    IMoviePlayWindowPlaybackRuntime& runtime) {
    return runtime.TogglePause(runtime.BinkPlayer());
}

std::uint8_t MoviePlayWindowPlayback::ProcessMovieEvent(
    IMoviePlayWindowPlaybackRuntime& runtime) {
    MoviePlaybackEventWords words{};
    runtime.HandleWindowEvent("MoviePlayWindow", words);
    words.first = 1u;
    const auto manager = runtime.FrameManager();
    return runtime.ProcessCommand(manager + 0x104u,
                                  "show_movie_preview", words);
}

std::uint8_t MoviePlayWindowPlayback::HandlePlaybackCommand(
    std::uint32_t event, IMoviePlayWindowPlaybackRuntime& runtime) {
    if (runtime.ReadWord(event) == 0u) {
        runtime.CloseVideo(runtime.BinkPlayer());
        return 1u;
    }

    MoviePlaybackEventWords words{1u, 0u};
    runtime.HandleWindowEvent("MoviePlayWindow", words);
    const auto manager = runtime.FrameManager();
    const auto frame = runtime.FindFrame(manager, "MoviePlayWindow");
    const auto record = runtime.ReadWord(event + 4u);
    const auto text_address = runtime.ReadWord(record + 0x28u);
    auto& state = runtime.FrameState(frame);
    runtime.InitializeVideoPath(state, "gamedata\\video\\");
    runtime.AppendVideoPath(
        state, text_address == 0u ? std::string_view{} :
                  runtime.CString(text_address));
    (void)Play(state, runtime);
    return 1u;
}

std::uint8_t MoviePlayWindowPlayback::StopButton(
    IMoviePlayWindowPlaybackRuntime& runtime) {
    runtime.CloseVideo(runtime.BinkPlayer());
    return 1u;
}

std::uint8_t MoviePlayWindowPlayback::PauseButton(
    IMoviePlayWindowPlaybackRuntime& runtime) {
    (void)TogglePause(runtime);
    return 1u;
}

std::uint8_t MoviePlayWindowPlayback::PlayButton(
    const MoviePlayWindowState& state,
    IMoviePlayWindowPlaybackRuntime& runtime) {
    (void)Play(state, runtime);
    return 1u;
}

}  // namespace pal4
