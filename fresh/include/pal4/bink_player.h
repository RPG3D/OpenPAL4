#pragma once

#include "pal4/bink_video.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace pal4 {

class IBinkPlayerRuntime {
public:
    virtual ~IBinkPlayerRuntime() = default;
    virtual void EnsureSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t id,
        std::function<std::byte*()> create,
        std::function<std::uint32_t(std::byte*)> destroy) = 0;
    [[nodiscard]] virtual std::byte* Allocate(std::size_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::byte* object) = 0;
    virtual void InitializePackage(std::byte* package) = 0;
    [[nodiscard]] virtual std::uint32_t ClosePackage(
        std::byte* package) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupPackage(
        std::byte* package) = 0;
};

class IBinkPlayerOpenRuntime : public IBinkPlayerRuntime,
                               public IBinkVideoRuntime {
public:
    [[nodiscard]] virtual float VideoMasterVolume() = 0;
    [[nodiscard]] virtual bool UseVideoPackages() = 0;
    [[nodiscard]] virtual std::string ScriptVideoName(
        std::uint32_t source) = 0;
    virtual void ResetPackageGlobalPointer(
        std::byte* package, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool FileExists(std::string_view path) = 0;
    [[nodiscard]] virtual std::string InstallRoot() = 0;
    virtual void PromptForDisk(char drive) = 0;
    [[nodiscard]] virtual bool OpenPackage(
        std::byte* package, std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t OpenPackageFile(
        std::byte* package, std::string_view path) = 0;
    virtual void SeekPackageFile(std::byte* package,
                                 std::uint32_t file,
                                 std::uint32_t offset,
                                 std::uint32_t origin) = 0;
    [[nodiscard]] virtual std::uint32_t PackageBinkSource(
        std::byte* package) = 0;
    virtual void ClosePackageFile(
        std::byte* package, std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t ViewportWidth() = 0;
    [[nodiscard]] virtual std::uint32_t ViewportHeight() = 0;
    [[nodiscard]] virtual std::uint32_t CreateTexture(
        std::uint32_t raster) = 0;
};

class IBinkPlayerPlaybackRuntime : public IBinkPlayerOpenRuntime {
public:
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    virtual void InvokeCloseCallback(std::uint32_t callback) = 0;
    virtual void LogSoundSetup(std::uint32_t phase) = 0;
    [[nodiscard]] virtual bool SetBinkSoundSystem(
        bool miles, std::uint32_t device) = 0;
    virtual void DrawTexturedRectangle(
        std::uint32_t context, std::uint32_t x,
        std::uint32_t y, std::uint32_t width,
        std::uint32_t height, std::uint32_t texture,
        std::uint32_t alpha, std::uint32_t red,
        std::uint32_t green, std::uint32_t blue) = 0;
};

class BinkPlayer final {
public:
    static constexpr std::size_t kSize = 0x200398u;
    static constexpr std::size_t kVideoOffset = 4u;
    static constexpr std::size_t kFirstPackageOffset = 28u;
    static constexpr std::size_t kSecondPackageOffset = 1049048u;
    static constexpr std::size_t kFirstPackageOpenFlag = 1048772u;
    static constexpr std::size_t kSecondPackageOpenFlag = 2097792u;

    // IDA 0x65cba0: register one guarded singleton under bnkBinkPlayer.
    [[nodiscard]] static std::uint32_t InitializeSingleton(
        std::uint8_t& guard, std::uint32_t& registration,
        IBinkPlayerRuntime& runtime);
    // IDA 0x65cbe0/0x65cc40: allocation and cleanup/free callbacks.
    [[nodiscard]] static std::byte* Create(IBinkPlayerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::byte* object, IBinkPlayerRuntime& runtime);
    // IDA 0x65cc60/0x65ccd0: initialize and clean nested package slots.
    [[nodiscard]] static std::byte* Initialize(
        std::byte* object, IBinkPlayerRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::byte* object, IBinkPlayerRuntime& runtime);
    // IDA 0x65cd50: open direct or package-backed video and set parameters.
    [[nodiscard]] static bool OpenVideoFile(
        std::byte* object, std::uint32_t source, bool center,
        IBinkPlayerOpenRuntime& runtime);
    // IDA 0x65d090..0x65d1f0: playback control, setup and rendering.
    [[nodiscard]] static std::uint32_t TogglePause(
        const std::byte* object, IBinkVideoRuntime& runtime);
    [[nodiscard]] static std::uint8_t CloseVideo(
        std::byte* object, IBinkPlayerPlaybackRuntime& runtime);
    [[nodiscard]] static bool SetSoundSystem(
        std::byte* object, bool miles, std::uint32_t device,
        IBinkPlayerPlaybackRuntime& runtime);
    [[nodiscard]] static std::uint8_t CleanupPlayback(
        std::byte* object, IBinkPlayerPlaybackRuntime& runtime);
    static void UpdateAndRender(
        std::byte* object, std::uint32_t context,
        IBinkPlayerPlaybackRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetVideoParameters(
        std::byte* object, bool native_size,
        IBinkPlayerOpenRuntime& runtime);
};

}  // namespace pal4
