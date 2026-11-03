#pragma once

#include "pal4/string_buffer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct AudioStreamState final {
    std::uint32_t vtable = 0u;
    std::uint32_t parent = 0u;
    std::uint32_t active = 0u;
    std::uint32_t stream = 0u;
    std::uint32_t loop_count = 0u;
    std::uint32_t volume_bits = 0u;
    std::uint32_t started_at_words[2]{};
    std::uint32_t fade_rate_bits = 0u;
    std::uint32_t fade_level_bits = 0u;
    StringBufferState path;
    std::uint32_t remaining_threshold_bits = 0u;
    std::uint32_t completion_callback = 0u;
    std::uint32_t temp_file = 0u;
};
static_assert(sizeof(AudioStreamState) == 64u);
static_assert(offsetof(AudioStreamState, path) == 40u);
static_assert(offsetof(AudioStreamState, completion_callback) == 56u);

class IAudioStreamRuntime {
public:
    virtual ~IAudioStreamRuntime() = default;
    virtual void InitializePath(AudioStreamState& stream) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupPath(
        AudioStreamState& stream) = 0;
};

class IAudioStreamControlRuntime : public IAudioStreamRuntime {
public:
    virtual void CloseMilesStream(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeCallback(
        std::uint32_t callback, std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t CloseTempFile(
        std::uint32_t file) = 0;
};

class IAudioStreamPauseRuntime {
public:
    virtual ~IAudioStreamPauseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PauseMilesStream(
        std::uint32_t stream, bool paused) = 0;
};

class IAudioStreamUpdateRuntime : public IAudioStreamControlRuntime {
public:
    [[nodiscard]] virtual std::uint32_t StreamStatus(
        std::uint32_t stream) = 0;
    virtual void StreamMilliseconds(std::uint32_t stream,
                                    std::uint32_t& total,
                                    std::uint32_t& current) = 0;
    [[nodiscard]] virtual float MasterVolume() = 0;
    [[nodiscard]] virtual std::uint32_t SetStreamVolume(
        std::uint32_t stream, std::uint32_t left_bits,
        std::uint32_t right_bits) = 0;
    virtual void ReadStreamVolume(std::uint32_t stream,
                                  std::uint32_t& left_bits,
                                  std::uint32_t& right_bits) = 0;
};

struct AudioOpenDevice final {
    std::uint32_t unknown0[5]{};
    std::uint32_t miles_driver = 0u;
};
static_assert(offsetof(AudioOpenDevice, miles_driver) == 20u);

struct AudioPackedFile final {
    std::uint32_t data = 0u;
    std::uint32_t bytes = 0u;
};

class IAudioStreamOpenRuntime : public IAudioStreamControlRuntime {
public:
    [[nodiscard]] virtual bool PackageMode() = 0;
    [[nodiscard]] virtual bool PackageAlreadyOpen() = 0;
    virtual void ClosePackage() = 0;
    virtual void SetPackagePointer(std::uint32_t value) = 0;
    [[nodiscard]] virtual bool OpenPackage(std::string_view path) = 0;
    [[nodiscard]] virtual std::string ResolveMusicPath(
        std::string_view path, std::uint32_t max_bytes) = 0;
    [[nodiscard]] virtual std::uint32_t OpenPackageFile(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t AllocatePackedBuffer(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t InitializePackedBuffer(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual AudioPackedFile LoadPackageFile(
        std::uint32_t file, std::uint32_t buffer) = 0;
    virtual void ClosePackageFile(std::uint32_t file) = 0;
    virtual void CleanupPackedBuffer(std::uint32_t buffer) = 0;
    virtual void FreePackedBuffer(std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::string TempDirectory(
        std::uint32_t max_bytes) = 0;
    [[nodiscard]] virtual std::uint32_t TimeMilliseconds() = 0;
    [[nodiscard]] virtual std::uint32_t Crc32(std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t CreateTempFile(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t WriteTempFile(
        std::uint32_t file, std::uint32_t data,
        std::uint32_t bytes) = 0;
    virtual void RewindTempFile(std::uint32_t file) = 0;
    virtual void RemovePath(std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t OpenMilesStream(
        std::uint32_t driver, std::string_view path) = 0;
    virtual void StartMilesStream(std::uint32_t stream) = 0;
    [[nodiscard]] virtual float MasterVolume() = 0;
    virtual void SetMilesVolume(std::uint32_t stream,
                                std::uint32_t left_bits,
                                std::uint32_t right_bits) = 0;
    virtual void SetMilesLoop(std::uint32_t stream,
                              std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint64_t PerformanceCounter() = 0;
    virtual void AssignPath(AudioStreamState& stream,
                            std::string_view path) = 0;
};

class AudioStream final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x84b534u;
    static constexpr std::uint32_t kStreamVtable = 0x84b504u;

    // IDA 0x65b670: construct base and stream state around path init.
    [[nodiscard]] static AudioStreamState& Initialize(
        AudioStreamState& stream, std::uint32_t parent,
        std::uint32_t parameter, IAudioStreamRuntime& runtime);
    // IDA 0x65b710: release stream, then path, restoring base vtable.
    [[nodiscard]] static std::uint32_t Cleanup(
        AudioStreamState& stream, IAudioStreamControlRuntime& runtime);
    // IDA 0x65bbd0: close active Miles stream, callback and temp file.
    [[nodiscard]] static std::uint32_t Release(
        AudioStreamState& stream, IAudioStreamControlRuntime& runtime);
    // IDA 0x65b770: open direct or package-backed Miles stream.
    [[nodiscard]] static std::uint32_t Open(
        AudioStreamState& stream, const char* path,
        const AudioOpenDevice* device, std::uint32_t loop_count,
        IAudioStreamOpenRuntime& runtime);
    // IDA 0x65bc20/0x65bc40: pause or resume a non-null Miles stream.
    [[nodiscard]] static std::uint32_t Pause(
        const AudioStreamState& stream, IAudioStreamPauseRuntime& runtime);
    [[nodiscard]] static std::uint32_t Resume(
        const AudioStreamState& stream, IAudioStreamPauseRuntime& runtime);
    // IDA 0x65bc60: report whether the stream active word is nonzero.
    [[nodiscard]] static bool IsActive(const AudioStreamState& stream);
    // IDA 0x65bc70: progress normal playback or fade state.
    [[nodiscard]] static std::uint32_t Update(
        AudioStreamState& stream, float seconds,
        IAudioStreamUpdateRuntime& runtime);
    // IDA 0x65bd90/0x65bde0: configure linear volume fades.
    [[nodiscard]] static std::uint32_t FadeOut(
        AudioStreamState& stream, float seconds,
        IAudioStreamUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t FadeIn(
        AudioStreamState& stream, float seconds,
        IAudioStreamUpdateRuntime& runtime);
    // IDA 0x65be40: apply equal Miles volume levels to a live stream.
    [[nodiscard]] static std::uint32_t SetVolume(
        const AudioStreamState& stream, std::uint32_t volume_bits,
        IAudioStreamUpdateRuntime& runtime);
};

}  // namespace pal4
