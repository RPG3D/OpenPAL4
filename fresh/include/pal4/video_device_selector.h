#pragma once

#include "pal4/game_config_initializer.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

class DisplayModeRecord final {
public:
    static constexpr std::uint32_t kBytes = 156u;

    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);
    void WriteWord(std::uint32_t offset, std::uint16_t value);
    [[nodiscard]] const std::array<std::uint8_t, kBytes>& Bytes() const {
        return bytes_;
    }
    void AssignBytes(const std::array<std::uint8_t, kBytes>& bytes) {
        bytes_ = bytes;
    }

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

struct VideoModeInfo {
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t depth = 0;
    std::uint32_t flags = 0u;
    std::int32_t refresh_rate = 0;
    std::int32_t format = 0;
};

static_assert(sizeof(VideoModeInfo) == 24u);
static_assert(offsetof(VideoModeInfo, flags) == 12u);

struct ClientRect {
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t right = 0;
    std::int32_t bottom = 0;
};

class IVideoDeviceRuntime : public IGameConfigLogger {
public:
    ~IVideoDeviceRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t GetNumSubSystems() = 0;
    virtual void GetSubSystemInfo(
        std::uint32_t index, std::array<std::uint32_t, 20>& output) = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentSubSystem() = 0;
    [[nodiscard]] virtual std::uint64_t ReadTimeStampCounter() = 0;
    virtual void EnumDisplaySettings(std::uint32_t mode_number,
                                     DisplayModeRecord& output) = 0;
    [[nodiscard]] virtual std::int32_t ChangeDisplaySettings(
        const DisplayModeRecord& mode, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentVideoMode() = 0;
    [[nodiscard]] virtual std::uint32_t SetSubSystem(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t SetVideoMode(
        std::uint32_t index) = 0;
    virtual void GetVideoModeInfo(std::uint32_t index,
                                  VideoModeInfo& output) = 0;
    virtual void SetWindowLong(std::uint32_t window, std::int32_t index,
                               std::uint32_t value) = 0;
    virtual void SetWindowPos(std::uint32_t window, std::uint32_t flags) = 0;
    virtual void GetClientRect(std::uint32_t window, ClientRect& output) = 0;
};

class VideoDeviceSelector final {
public:
    VideoDeviceSelector(GameConfigWords& config, GameRenderState& state)
        : config_(config), state_(state) {}

    [[nodiscard]] std::uint32_t Select(IVideoDeviceRuntime& runtime);

private:
    GameConfigWords& config_;
    GameRenderState& state_;
};

}  // namespace pal4
