#include "pal4/script_movie_playback.h"

#include <string>

namespace pal4 {

std::uint32_t ScriptMoviePlayback::Play(std::string_view movie_name) {
    return PlayWithMode(movie_name, false);
}

std::uint32_t ScriptMoviePlayback::PlayFinal(std::string_view movie_name) {
    return PlayWithMode(movie_name, true);
}

std::uint32_t ScriptMoviePlayback::PlayWithMode(
    std::string_view movie_name, bool final_movie) {
    const auto first_player = runtime_.Player();
    if (!runtime_.PlayerReady(first_player)) {
        const auto first_audio = runtime_.AudioManager();
        if (!runtime_.AudioReady(first_audio))
            runtime_.InitializeAudio(runtime_.AudioManager(), 0u);
        const auto device = runtime_.AudioDevice(runtime_.AudioManager());
        runtime_.SetSoundSystem(runtime_.Player(), 1u, device);
    }

    const auto path = std::string("gamedata\\video\\") +
                      std::string(movie_name);
    runtime_.CloseVideo(runtime_.Player());
    runtime_.OpenVideo(runtime_.Player(), path, 1u);
    runtime_.InstallCompletionCallback(
        runtime_.Player(), kCallbackOffset, kCallbackAddress);

    auto* state = runtime_.AllocateWait(sizeof(MovieWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, 1u,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kWaitVtable;
        state->return_to_game = final_movie ? 1u : 0u;
        shared_.finished = 0u;
        runtime_.FlushWait(*state);
        state->saved_volume = runtime_.ReadVolume(runtime_.AudioManager());
        const auto audio = runtime_.AudioManager();
        runtime_.WriteVolume(audio, 0.0f);
        RefreshTypeTwoChannels(audio);
    }
    runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, state);

    const auto document = runtime_.CreateStateDocument();
    static_cast<void>(runtime_.EnterState(12u, document));
    if (final_movie)
        static_cast<void>(runtime_.EnterState(12u, document));
    runtime_.DestroyStateDocument(document);
    return runtime_.CleanupPath();
}

std::uint8_t ScriptMoviePlayback::Update(MovieWaitState& state) {
    if (shared_.finished == 0u) return 0u;
    runtime_.CloseWait(state);
    const auto audio = runtime_.AudioManager();
    runtime_.WriteVolume(audio, state.saved_volume);
    RefreshTypeTwoChannels(audio);
    return ReturnToGame(state);
}

std::uint8_t ScriptMoviePlayback::Stop(MovieWaitState& state) {
    runtime_.CloseWait(state);
    shared_.finished = 1u;
    runtime_.CloseVideo(runtime_.Player());
    const auto audio = runtime_.AudioManager();
    runtime_.WriteVolume(audio, state.saved_volume);
    RefreshTypeTwoChannels(audio);
    return ReturnToGame(state);
}

void ScriptMoviePlayback::MarkFinished() {
    shared_.finished = 1u;
}

void ScriptMoviePlayback::RefreshTypeTwoChannels(std::uint32_t audio) {
    for (std::uint32_t i = 0, count = runtime_.ChannelCount(audio);
         i < count; ++i) {
        const auto channel = runtime_.ChannelAt(audio, i);
        if (channel != 0u && runtime_.ChannelType(channel) == 2u)
            runtime_.InvokeChannelSlot44(channel);
    }
}

std::uint8_t ScriptMoviePlayback::ReturnToGame(
    const MovieWaitState& state) {
    if (state.return_to_game == 0u) return 0u;
    runtime_.ResetScreenEffect();
    return runtime_.EnterState(0u, 0u);
}

}  // namespace pal4
