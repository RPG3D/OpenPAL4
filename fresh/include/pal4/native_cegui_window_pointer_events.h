#pragma once

#include "pal4/cegui_window_pointer_events.h"
#include <windows.h>

namespace pal4 {
// Borrows original CEGUI Window objects. This dispatches to their DLL virtual
// methods without moving the OS cursor, activating a window or posting input.
class NativeCeguiWindowPointerEvents final : private ICeguiWindowPointerRuntime {
public:
    explicit NativeCeguiWindowPointerEvents(HMODULE module);
    [[nodiscard]] CeguiPoint AbsolutePosition(std::uint32_t window);
    [[nodiscard]] std::uint32_t HandleMouseAtCenter(std::uint32_t window);
private:
    std::uint32_t ReadWord(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    CeguiPoint ParentSize(std::uint32_t window) override;
    double PixelAlignOffset() override;
    std::int32_t FloatToInt(double value) override;
    std::uint32_t MouseEventVtable() override;
    std::uint32_t InvokeWindowEvent(std::uint32_t window,
        std::uint32_t slot_offset, CeguiMouseEvent& event) override;
    HMODULE module_;
};
}  // namespace pal4
