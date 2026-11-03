#include "pal4/audio_processing_controller.h"

namespace pal4 {

std::uint8_t AudioProcessingController::SetProcessingState(
    const std::uint8_t enabled, IAudioProcessingRuntime& runtime) {
    std::uint32_t eax = enabled;
    if (state_.enabled == enabled) return enabled;
    state_.enabled = enabled;

    std::uint32_t manager = 0u;
    if (enabled != 0u) {
        const auto source = runtime.AudioManagerInstance();
        state_.saved_volume_bits = runtime.ReadWord(source);
        manager = runtime.AudioManagerInstance();
        eax = manager;
        runtime.WriteWord(manager, 0u);
    } else {
        manager = runtime.AudioManagerInstance();
        eax = manager;
        runtime.WriteWord(manager, state_.saved_volume_bits);
    }

    for (std::uint32_t index = 0u;; ++index) {
        const auto begin = runtime.ReadWord(manager + kStreamsBeginOffset);
        if (begin == 0u) return static_cast<std::uint8_t>(eax);
        const auto end = runtime.ReadWord(manager + kStreamsEndOffset);
        const auto count = (end - begin) >> 2u;
        eax = count;
        if (index >= count) return static_cast<std::uint8_t>(eax);

        eax = begin;
        const auto stream = runtime.ReadWord(begin + index * 4u);
        if (stream != 0u)
            eax = runtime.SetStreamVolume(stream, runtime.ReadWord(manager));
    }
}

}  // namespace pal4
