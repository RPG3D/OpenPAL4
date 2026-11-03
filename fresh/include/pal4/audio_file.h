#pragma once

#include "pal4/string_buffer.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct AudioFileState final {
    std::uint32_t path_crc = 0u;
    std::uint32_t data = 0u;
    std::uint32_t bytes = 0u;
    std::uint32_t reference_count = 0u;
    StringBufferState path;
};
static_assert(sizeof(AudioFileState) == 28u);
static_assert(offsetof(AudioFileState, path) == 16u);

using AudioWaveInfo = std::array<std::uint8_t, 36u>;

class IAudioFileRuntime {
public:
    virtual ~IAudioFileRuntime() = default;
    virtual void InitializePath(StringBufferState& path) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupPath(
        StringBufferState& path) = 0;
    virtual void AssignPath(StringBufferState& path, const char* text,
                            std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t FileSize(const char* path) = 0;
    [[nodiscard]] virtual std::uint32_t FileRead(const char* path,
                                                 std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t FileType(std::uint32_t data,
                                                 std::uint32_t bytes) = 0;
    virtual void WaveInfo(std::uint32_t data, AudioWaveInfo& info) = 0;
    [[nodiscard]] virtual bool DecompressAdpcm(
        const AudioWaveInfo& info, std::uint32_t& output,
        std::uint32_t& bytes) = 0;
    [[nodiscard]] virtual bool DecompressAsi(
        std::uint32_t data, std::uint32_t bytes, const char* extension,
        std::uint32_t& output, std::uint32_t& output_bytes,
        std::uint32_t flags) = 0;
    virtual void FreeLocked(std::uint32_t data) = 0;
    virtual void Log(std::uint32_t message_id) = 0;
    [[nodiscard]] virtual std::uint32_t ComputeCrc(
        const char* path, std::uint32_t length) = 0;
};

class IAudioFileDestroyRuntime : public IAudioFileRuntime {
public:
    [[nodiscard]] virtual std::uint32_t FreeFile(
        std::uint32_t address) = 0;
};

class AudioFile final {
public:
    // IDA 0x65a340: clear selected words and initialize the path string.
    [[nodiscard]] static AudioFileState& Initialize(
        AudioFileState& file, IAudioFileRuntime& runtime);
    // IDA 0x65a360: release locked data, then cleanup path ownership.
    [[nodiscard]] static std::uint32_t Cleanup(
        AudioFileState& file, IAudioFileRuntime& runtime);
    // IDA 0x65a380: load raw, ADPCM or MP3 data and store path metadata.
    [[nodiscard]] static std::uint8_t Load(
        AudioFileState& file, const char* path, IAudioFileRuntime& runtime);
    // IDA 0x65aa30: cleanup file and optionally free its object allocation.
    [[nodiscard]] static std::uint32_t DestroyAt(
        AudioFileState& file, std::uint32_t address, std::uint8_t flags,
        IAudioFileDestroyRuntime& runtime);
};

}  // namespace pal4
