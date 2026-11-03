#pragma once

#include "pal4/string_buffer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct AudioSampleState final {
    std::uint32_t vtable = 0u;
    std::uint32_t parent = 0u;
    std::uint32_t audio_file = 0u;
    std::uint32_t sample = 0u;
    std::uint32_t loop_count = 0u;
    std::uint32_t volume_bits = 0u;
    std::uint32_t started_at_words[2]{};
    StringBufferState path;
};
static_assert(sizeof(AudioSampleState) == 44u);
static_assert(offsetof(AudioSampleState, path) == 32u);

class IAudioSampleRuntime {
public:
    virtual ~IAudioSampleRuntime() = default;
    virtual void InitializePath(AudioSampleState& sample) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupPath(
        AudioSampleState& sample) = 0;
    [[nodiscard]] virtual std::uint32_t FileCrc(
        std::uint32_t audio_file) = 0;
    virtual void ReleasePoolFile(std::uint32_t crc) = 0;
};

class IAudioSampleControlRuntime : public IAudioSampleRuntime {
public:
    virtual void EndMilesSample(std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint32_t RecycleMilesSample(
        std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint32_t StopMilesSample(
        std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint32_t ResumeMilesSample(
        std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint32_t MilesSampleStatus(
        std::uint32_t sample) = 0;
    [[nodiscard]] virtual float SampleMasterVolume() = 0;
    [[nodiscard]] virtual std::uint32_t SetMilesSampleVolume(
        std::uint32_t sample, std::uint32_t left_bits,
        std::uint32_t right_bits) = 0;
};

class IAudioSamplePlayRuntime : public IAudioSampleControlRuntime {
public:
    virtual void InitializeMilesSample(std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::string FilePath(
        std::uint32_t audio_file) = 0;
    [[nodiscard]] virtual std::uint32_t AddPoolFile(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t FileData(
        std::uint32_t audio_file) = 0;
    [[nodiscard]] virtual bool SetMilesSampleFile(
        std::uint32_t sample, std::uint32_t data) = 0;
    virtual void SetMilesSampleLoop(std::uint32_t sample,
                                    std::uint32_t loop_count) = 0;
    virtual void StartMilesSample(std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint64_t PerformanceCounter() = 0;
    virtual void AssignPath(AudioSampleState& sample,
                            std::string_view path) = 0;
};

class AudioSample final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x84b534u;
    static constexpr std::uint32_t kSampleVtable = 0x84b568u;

    // IDA 0x65be60: construct sample object and path under base vtable.
    [[nodiscard]] static AudioSampleState& Initialize(
        AudioSampleState& sample, std::uint32_t parent,
        IAudioSampleRuntime& runtime);
    // IDA 0x65bef0: stop sample, release pool file and cleanup path.
    [[nodiscard]] static std::uint32_t Cleanup(
        AudioSampleState& sample, IAudioSampleControlRuntime& runtime);
    // IDA 0x65bf60: bind cached/loaded file data to a Miles sample.
    [[nodiscard]] static std::uint32_t Play(
        AudioSampleState& sample, std::uint32_t miles_sample,
        const char* path, const void* device,
        std::uint32_t loop_count, IAudioSamplePlayRuntime& runtime);
    // IDA 0x65c0e0: end and recycle a live Miles sample.
    [[nodiscard]] static std::uint32_t Stop(
        AudioSampleState& sample, IAudioSampleControlRuntime& runtime);
    // IDA 0x65c110: raw Miles stop, including a zero handle.
    [[nodiscard]] static std::uint32_t StopImmediate(
        const AudioSampleState& sample,
        IAudioSampleControlRuntime& runtime);
    // IDA 0x65c120/0x65c130: resume and query sample status.
    [[nodiscard]] static std::uint32_t Resume(
        const AudioSampleState& sample,
        IAudioSampleControlRuntime& runtime);
    [[nodiscard]] static bool IsPlaying(
        const AudioSampleState& sample,
        IAudioSampleControlRuntime& runtime);
    // IDA 0x65c150: apply master times local volume to both channels.
    [[nodiscard]] static std::uint32_t ApplyVolume(
        const AudioSampleState& sample,
        IAudioSampleControlRuntime& runtime);
};

}  // namespace pal4
