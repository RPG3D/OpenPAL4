#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <array>

namespace pal4 {

class NativeMilesDigitalDriver;

// Plays an original Music/*.smp member through the shared Miles driver.
// The driver must outlive this stream.
class NativeMilesMusicStream final {
public:
    NativeMilesMusicStream(std::filesystem::path game_root,
                           NativeMilesDigitalDriver& driver);
    ~NativeMilesMusicStream();
    NativeMilesMusicStream(const NativeMilesMusicStream&) = delete;
    NativeMilesMusicStream& operator=(const NativeMilesMusicStream&) = delete;

    [[nodiscard]] bool Open(std::string_view track, unsigned loop_count = 0u);
    [[nodiscard]] bool Pause();
    [[nodiscard]] bool Resume();
    [[nodiscard]] bool FadeIn(float seconds);
    [[nodiscard]] bool FadeOut(float seconds);
    // Original AudioHandle::SetProperty writes stream+52. The shared stream
    // update compares this threshold with remaining time before notification.
    void SetCompletionThreshold(float seconds);
    [[nodiscard]] float CompletionThreshold() const;
    void Update(float seconds);
    void Stop();
    void SetVolume(float value);
    [[nodiscard]] std::array<float, 2> VolumeLevels() const;
    [[nodiscard]] bool Active() const;
    [[nodiscard]] const std::string& LastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace pal4
