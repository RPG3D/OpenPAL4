#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace pal4 {
// Owns the three original PalFontManager fonts through the audited D3DX9
// adapter. Construct after RenderWare Start; destroy before device shutdown.
class NativePalFontManager final {
public:
    using Log = std::function<void(std::uint32_t)>;
    explicit NativePalFontManager(Log);
    ~NativePalFontManager();
    NativePalFontManager(const NativePalFontManager&) = delete;
    NativePalFontManager& operator=(const NativePalFontManager&) = delete;
    int Draw(std::string_view text, const std::array<std::int32_t, 4>& rectangle,
        std::uint32_t font_index, std::uint32_t color, std::uint32_t flags);
    void OnLostDevice();
    void OnResetDevice();
    void Release();
    std::uint32_t FontCount() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
