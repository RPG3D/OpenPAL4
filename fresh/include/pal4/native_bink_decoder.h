#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace pal4 {

struct NativeBinkFrameInfo {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t frame_count{};
    std::uint32_t frame_number{};
};

class NativeBinkDecoder final {
public:
    explicit NativeBinkDecoder(const char* dll_path);
    ~NativeBinkDecoder();
    NativeBinkDecoder(const NativeBinkDecoder&) = delete;
    NativeBinkDecoder& operator=(const NativeBinkDecoder&) = delete;

    [[nodiscard]] bool OpenFile(const char* path);
    [[nodiscard]] bool ConfigureMilesSound(void* digital_driver);
    void Close();
    [[nodiscard]] NativeBinkFrameInfo FrameInfo() const;
    [[nodiscard]] bool WaitForFrame() const;
    [[nodiscard]] bool DecodeFrame(std::span<std::uint8_t> bgra_pixels);
    [[nodiscard]] bool NextFrame();
    [[nodiscard]] std::string LastError() const;

private:
    void* library_{};
    void* movie_{};
    void* open_{};
    void* close_{};
    void* do_frame_{};
    void* wait_{};
    void* copy_to_buffer_{};
    void* next_frame_{};
    void* get_error_{};
    void* set_sound_system_{};
    void* open_miles_{};
};

}  // namespace pal4
