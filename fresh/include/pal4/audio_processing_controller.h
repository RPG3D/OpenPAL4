#pragma once

#include <cstdint>

namespace pal4 {

struct AudioProcessingState {
    std::uint32_t saved_volume_bits = 0u;
    std::uint8_t enabled = 0u;
};

class IAudioProcessingRuntime {
public:
    virtual ~IAudioProcessingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AudioManagerInstance() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SetStreamVolume(
        std::uint32_t stream, std::uint32_t volume_bits) = 0;
};

class AudioProcessingController final {
public:
    static constexpr std::uint32_t kSavedVolumeAddress = 0x8f2d78u;
    static constexpr std::uint32_t kStateAddress = 0x8f2d7cu;
    static constexpr std::uint32_t kStreamsBeginOffset = 36u;
    static constexpr std::uint32_t kStreamsEndOffset = 40u;

    explicit AudioProcessingController(AudioProcessingState& state)
        : state_(state) {}

    // IDA 0x513e40.
    [[nodiscard]] std::uint8_t SetProcessingState(
        std::uint8_t enabled, IAudioProcessingRuntime& runtime);

private:
    AudioProcessingState& state_;
};

}  // namespace pal4
