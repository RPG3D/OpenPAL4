#include "pal4/script_sound_2d.h"

#include <string>

namespace pal4 {

std::int32_t ScriptSound2d::Play(
    std::uint32_t source, std::int32_t mode,
    IScriptSound2dRuntime& runtime) {
    std::string path = "gamedata\\PALSound\\";
    path.append(runtime.SoundName(source));
    path.append(".wav");

    if (mode < 0) {
        // IDA5D59B1 writes the AudioHandle into the former source argument.
        const auto handle = runtime.PlaySound(runtime.AudioManager(), source, path, 0);
        return runtime.InsertSoundId(runtime.ScriptExecutor(), handle);
    }

    const auto executor = runtime.ScriptExecutor();
    const auto handle = runtime.PlaySound(runtime.AudioManager(), source,
                                          path, mode);
    runtime.SetCurrentHandle(executor + 0x20u, handle);
    return -1;
}

std::uint32_t ScriptSound2d::StopCurrent(
    IScriptSound2dRuntime& runtime) {
    const auto active = runtime.CheckActive(runtime.ScriptExecutor() + 0x20u);
    if ((active & 0xffu) == 0u) return active;
    return runtime.StopCurrent(runtime.ScriptExecutor() + 0x20u);
}

std::uint32_t ScriptSound2d::StopId(
    std::int32_t id, IScriptSound2dRuntime& runtime) {
    if (id < 0) return static_cast<std::uint32_t>(id);
    return runtime.RemoveSoundId(runtime.ScriptExecutor(), id);
}

}  // namespace pal4
