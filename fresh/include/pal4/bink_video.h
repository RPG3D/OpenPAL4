#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct BinkVideoState final {
    std::uint32_t handle = 0u;
    std::uint32_t raster = 0u;
    std::uint32_t copy_mode = 0u;
    std::uint32_t offset_x_bits = 0u;
    std::uint32_t offset_y_bits = 0u;
};
static_assert(sizeof(BinkVideoState) == 20u);
static_assert(offsetof(BinkVideoState, copy_mode) == 8u);

class IBinkVideoRuntime {
public:
    virtual ~IBinkVideoRuntime() = default;
    [[nodiscard]] virtual std::uint32_t OpenBink(
        std::uint32_t source, bool use_io_table) = 0;
    [[nodiscard]] virtual std::uint32_t CloseBink(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t SetBinkVolume(
        std::uint32_t handle, std::uint32_t track,
        std::uint32_t scaled_volume) = 0;
    [[nodiscard]] virtual std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t type) = 0;
    [[nodiscard]] virtual std::uint32_t VideoWidth(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t VideoHeight(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint8_t RasterFlags(
        std::uint32_t raster) = 0;
    virtual void LogFailure(std::uint32_t code) = 0;
    [[nodiscard]] virtual bool IsPaused(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t PauseBink(
        std::uint32_t handle, bool pause) = 0;
    [[nodiscard]] virtual std::uint32_t FrameNumber(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t FrameCount(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t WaitBink(
        std::uint32_t handle) = 0;
    virtual void DoFrame(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t RasterWidth(
        std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t RasterStride(
        std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t LockRaster(
        std::uint32_t raster, std::uint32_t level,
        std::uint32_t mode) = 0;
    virtual void CopyToBuffer(
        std::uint32_t handle, std::uint32_t pixels,
        std::uint32_t stride, std::uint32_t width,
        std::int32_t x, std::int32_t y, std::uint32_t flags) = 0;
    virtual void UnlockRaster(std::uint32_t raster) = 0;
    virtual void NextFrame(std::uint32_t handle) = 0;
};

class BinkVideo final {
public:
    // IDA 0x65c760: reset handle, raster, and offsets while preserving mode.
    [[nodiscard]] static BinkVideoState& Initialize(BinkVideoState& state);
    // IDA 0x65c810/0x65c860: replace the video and apply volume on success.
    [[nodiscard]] static bool Open(
        BinkVideoState& state, std::uint32_t source, float volume,
        bool use_io_table, IBinkVideoRuntime& runtime);
    [[nodiscard]] static bool OpenDefault(
        BinkVideoState& state, std::uint32_t source, float volume,
        IBinkVideoRuntime& runtime);
    // IDA 0x65c880/0x65c940: attach raster and configure centered copy.
    [[nodiscard]] static std::uint32_t SetPlaybackMode(
        BinkVideoState& state, std::uint32_t raster,
        IBinkVideoRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetParameters(
        BinkVideoState& state, std::uint32_t width,
        std::uint32_t height, bool center,
        IBinkVideoRuntime& runtime);
    // IDA 0x65ca00/0x65ca20: pause toggle and resource teardown.
    [[nodiscard]] static std::uint32_t TogglePause(
        const BinkVideoState& state, IBinkVideoRuntime& runtime);
    [[nodiscard]] static std::uint32_t Close(
        BinkVideoState& state, IBinkVideoRuntime& runtime);
    // IDA 0x65ca40..0x65cb10: playback, volume and frame transfer.
    [[nodiscard]] static bool Update(
        BinkVideoState& state, IBinkVideoRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetVolume(
        const BinkVideoState& state, float volume,
        IBinkVideoRuntime& runtime);
    [[nodiscard]] static bool IsPlaying(
        const BinkVideoState& state, IBinkVideoRuntime& runtime);
    [[nodiscard]] static bool WaitFrame(
        const BinkVideoState& state, IBinkVideoRuntime& runtime);
    [[nodiscard]] static bool RenderFrame(
        BinkVideoState& state, IBinkVideoRuntime& runtime);
};

}  // namespace pal4
