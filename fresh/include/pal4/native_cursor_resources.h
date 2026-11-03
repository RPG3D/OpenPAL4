#pragma once

#include <windows.h>

#include <array>
#include <filesystem>
#include <string_view>

namespace pal4 {

class INativeInteractionCursorService {
public:
    virtual ~INativeInteractionCursorService() = default;
    virtual void SetCursor(std::string_view name) = 0;
};

// Owns the four uiFrameManager_Initialize cursor resources for one game window.
class NativeCursorResources final : public INativeInteractionCursorService {
public:
    NativeCursorResources(const std::filesystem::path& game_root, HWND window);
    ~NativeCursorResources() override;
    NativeCursorResources(const NativeCursorResources&) = delete;
    NativeCursorResources& operator=(const NativeCursorResources&) = delete;

    void SetCursor(std::string_view name) override;
    [[nodiscard]] HCURSOR DefaultCursor() const noexcept { return cursors_[0]; }
    [[nodiscard]] std::string_view CurrentName() const noexcept { return current_name_; }

private:
    HWND window_;
    std::array<HCURSOR, 4> cursors_{};
    std::string_view current_name_;
};

}  // namespace pal4
