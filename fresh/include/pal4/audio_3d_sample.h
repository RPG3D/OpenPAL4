#pragma once

#include "pal4/string_buffer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct Audio3DSampleState final {
    std::uint32_t vtable = 0u;
    std::uint32_t parent = 0u;
    std::uint32_t audio_file = 0u;
    std::uint32_t sample = 0u;
    std::uint32_t volume_bits = 0u;
    std::uint32_t loop_count = 0u;
    std::array<std::uint32_t, 3u> vector24{};
    std::array<std::uint32_t, 3u> vector36{};
    std::array<std::uint32_t, 3u> vector48{};
    std::uint32_t range60_bits = 0u;
    std::uint32_t range64_bits = 0u;
    std::uint32_t unknown68 = 0u;
    std::uint32_t started_at_words[2]{};
    StringBufferState path;
};
static_assert(sizeof(Audio3DSampleState) == 92u);
static_assert(offsetof(Audio3DSampleState, vector24) == 24u);
static_assert(offsetof(Audio3DSampleState, vector36) == 36u);
static_assert(offsetof(Audio3DSampleState, vector48) == 48u);
static_assert(offsetof(Audio3DSampleState, started_at_words) == 72u);
static_assert(offsetof(Audio3DSampleState, path) == 80u);

class IAudio3DSampleRuntime {
public:
    virtual ~IAudio3DSampleRuntime() = default;
    [[nodiscard]] virtual std::array<std::uint32_t, 3u> DefaultVector() = 0;
    virtual void InitializePath(Audio3DSampleState& sample) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupPath(
        Audio3DSampleState& sample) = 0;
    virtual void Stop(Audio3DSampleState& sample) = 0;
    [[nodiscard]] virtual std::uint32_t FileCrc(
        std::uint32_t audio_file) = 0;
    virtual void ReleasePoolFile(std::uint32_t crc) = 0;
    [[nodiscard]] virtual std::uint32_t FreeObject(
        std::uint32_t address) = 0;
};

class IAudio3DSampleControlRuntime : public IAudio3DSampleRuntime {
public:
    virtual void EndMiles3DSample(std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint32_t RecycleMiles3DSample(
        std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint32_t StopMiles3DSample(
        std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint32_t ResumeMiles3DSample(
        std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint32_t Miles3DSampleStatus(
        std::uint32_t sample) = 0;
    [[nodiscard]] virtual float SampleMasterVolume() = 0;
    [[nodiscard]] virtual std::uint32_t SetMiles3DSampleVolume(
        std::uint32_t sample, std::uint32_t bits) = 0;
    [[nodiscard]] virtual std::uint32_t SetMiles3DPosition(
        std::uint32_t sample, std::uint32_t x, std::uint32_t y,
        std::uint32_t z) = 0;
};

class IAudio3DSamplePlayRuntime : public IAudio3DSampleControlRuntime {
public:
    [[nodiscard]] virtual std::string FilePath(std::uint32_t audio_file) = 0;
    [[nodiscard]] virtual std::uint32_t AddPoolFile(std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t FileData(std::uint32_t audio_file) = 0;
    [[nodiscard]] virtual bool ReadWavInfo(std::uint32_t data,
                                           std::uint32_t& channels) = 0;
    virtual void LogWavFailure(std::uint32_t code) = 0;
    virtual void ReleaseMiles3DSample(std::uint32_t sample) = 0;
    [[nodiscard]] virtual bool SetMiles3DSampleFile(std::uint32_t sample,
                                                     std::uint32_t data) = 0;
    virtual void SetMiles3DSampleLoop(std::uint32_t sample,
                                       std::uint32_t count) = 0;
    virtual void SetMiles3DSampleDistances(std::uint32_t sample,
                                             std::uint32_t near_bits,
                                             std::uint32_t far_bits) = 0;
    virtual void SetMiles3DSampleCone(std::uint32_t sample,
                                       std::uint32_t inner_bits,
                                       std::uint32_t outer_bits,
                                       std::uint32_t outside_bits) = 0;
    virtual void SetMiles3DVelocity(std::uint32_t sample,
                                     std::uint32_t x, std::uint32_t y,
                                     std::uint32_t z,
                                     std::uint32_t scale_bits) = 0;
    virtual void AutoUpdateMiles3DPosition(std::uint32_t sample,
                                             std::uint32_t enabled) = 0;
    virtual void StartMiles3DSample(std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint64_t PerformanceCounter() = 0;
    virtual void AssignPath(Audio3DSampleState& sample,
                            std::string_view path) = 0;
};

class Audio3DSample final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x84b534u;
    static constexpr std::uint32_t k3DVtable = 0x84b59cu;

    // IDA 0x65c190: construct 3D sample and initialize its three vectors.
    [[nodiscard]] static Audio3DSampleState& Initialize(
        Audio3DSampleState& sample, std::uint32_t parent,
        IAudio3DSampleRuntime& runtime);
    // IDA 0x65c260: cleanup and optionally free object allocation.
    [[nodiscard]] static std::uint32_t DestroyAt(
        Audio3DSampleState& sample, std::uint32_t address,
        std::uint8_t flags, IAudio3DSampleRuntime& runtime);
    // IDA 0x65c280: stop, release audio-file reference, cleanup path.
    [[nodiscard]] static std::uint32_t Cleanup(
        Audio3DSampleState& sample, IAudio3DSampleRuntime& runtime);
    // IDA 0x65c2f0: forward the object's distance limits to PlayWithDistances.
    [[nodiscard]] static std::uint32_t Play(
        Audio3DSampleState& sample, std::uint32_t miles_sample,
        const char* path, const void* device, std::uint32_t loop_count,
        IAudio3DSamplePlayRuntime& runtime);
    // IDA 0x65c320: bind mono WAV data and configure spatial playback.
    [[nodiscard]] static std::uint32_t PlayWithDistances(
        Audio3DSampleState& sample, std::uint32_t miles_sample,
        const char* path, const void* device, std::uint32_t loop_count,
        std::uint32_t near_bits, std::uint32_t far_bits,
        IAudio3DSamplePlayRuntime& runtime);
    // IDA 0x65c5c0..0x65c670: Miles 3D sample controls.
    [[nodiscard]] static std::uint32_t Stop(
        Audio3DSampleState& sample, IAudio3DSampleControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t StopImmediate(
        const Audio3DSampleState& sample,
        IAudio3DSampleControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t Resume(
        const Audio3DSampleState& sample,
        IAudio3DSampleControlRuntime& runtime);
    [[nodiscard]] static bool IsPlaying(
        const Audio3DSampleState& sample,
        IAudio3DSampleControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPosition(
        Audio3DSampleState& sample,
        const std::array<std::uint32_t, 3u>& vector,
        IAudio3DSampleControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyVolume(
        const Audio3DSampleState& sample,
        IAudio3DSampleControlRuntime& runtime);
};

}  // namespace pal4
