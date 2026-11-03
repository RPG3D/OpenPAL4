#pragma once
#include <cstdint>
#include <memory>

namespace pal4 {
class NativePalFontManager;
class PalConsoleState;
// Borrows console state/fonts; owns the filled-rectangle texture. This renders
// the shared console algorithm, while command/input/history ownership is separate.
class NativePalConsoleRenderer final {
public:
    NativePalConsoleRenderer(PalConsoleState&, NativePalFontManager&);
    ~NativePalConsoleRenderer();
    NativePalConsoleRenderer(const NativePalConsoleRenderer&) = delete;
    NativePalConsoleRenderer& operator=(const NativePalConsoleRenderer&) = delete;
    // Frame is the active RwCamera* slot address, as for other screen draws.
    void Render(std::uint32_t frame);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
