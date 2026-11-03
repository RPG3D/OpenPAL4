#include "pal4/script_music_transport.h"

namespace pal4 {

std::uint8_t ScriptMusicTransport::Pause(
    IScriptMusicTransportRuntime& runtime) {
    return runtime.StopSound(runtime.BackgroundMusicPlayer() + 0x30u);
}

std::uint8_t ScriptMusicTransport::Resume(
    IScriptMusicTransportRuntime& runtime) {
    return runtime.LoadAndPlay(runtime.BackgroundMusicPlayer());
}

}  // namespace pal4
