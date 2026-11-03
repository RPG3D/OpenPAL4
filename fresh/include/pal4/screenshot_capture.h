#pragma once

#include "pal4/renderware_image.h"

#include <cstdint>
#include <string>

namespace pal4 {

struct ScreenshotTime {
    std::int32_t year_from_1900{};
    std::int32_t month_zero_based{};
    std::int32_t day{};
    std::int32_t hour{};
    std::int32_t minute{};
    std::int32_t second{};
};

class IScreenshotCaptureRuntime {
public:
    virtual ~IScreenshotCaptureRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeCameraSubsystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObject(
        std::uint32_t subsystem) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentTime() = 0;
    [[nodiscard]] virtual ScreenshotTime LocalTime(
        std::uint32_t time) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeFilename() = 0;
    [[nodiscard]] virtual std::string FormatFilename(
        std::uint32_t filename, const char* format,
        std::int32_t year, std::int32_t month, std::int32_t day,
        std::int32_t hour, std::int32_t minute,
        std::int32_t second) = 0;
    [[nodiscard]] virtual std::string CurrentDirectory(
        std::uint32_t buffer_size) = 0;
    [[nodiscard]] virtual bool SetCurrentDirectory(
        const std::string& path) = 0;
    virtual void CreateDirectory(const std::string& path) = 0;
    virtual void DeleteFile(const std::string& path) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupFilename(
        std::uint32_t filename) = 0;
};

class ScreenshotCapture final {
public:
    // IDA 0x4bff50; the function body continues through 0x4c008a.
    [[nodiscard]] static std::uint32_t Capture(
        IScreenshotCaptureRuntime& runtime,
        IRenderWareImageApi& renderware);
};

}  // namespace pal4
