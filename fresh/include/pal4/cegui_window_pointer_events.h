#pragma once

#include <cstdint>

namespace pal4 {

struct CeguiPoint {
    float x = 0.0f;
    float y = 0.0f;
};

struct CeguiMouseEvent {
    std::uint32_t vtable = 0;
    std::uint8_t handled = 0;
    std::uint32_t window = 0;
    CeguiPoint position{};
    CeguiPoint move_delta{};
    // The installed DLL reads button at+1C; the previous clicks label at this
    // offset was incorrect. Keep the remaining original helper words explicit.
    std::uint32_t button = 0;
    std::uint32_t state_a = 1;
    std::uint32_t state_b = 0;
    std::uint32_t state_c = 1;
};

class ICeguiWindowPointerRuntime {
public:
    virtual ~ICeguiWindowPointerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual CeguiPoint ParentSize(std::uint32_t window) = 0;
    [[nodiscard]] virtual double PixelAlignOffset() = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(double value) = 0;
    [[nodiscard]] virtual std::uint32_t MouseEventVtable() = 0;
    [[nodiscard]] virtual std::uint32_t InvokeWindowEvent(
        std::uint32_t window, std::uint32_t slot_offset,
        CeguiMouseEvent& event) = 0;
};

class CeguiWindowPointerEvents final {
public:
    // IDA 0x4af0b0.
    [[nodiscard]] static CeguiPoint AbsolutePosition(
        std::uint32_t window, ICeguiWindowPointerRuntime& runtime);
    // IDA 0x4af160.
    [[nodiscard]] static std::uint32_t HandleMouseAtCenter(
        std::uint32_t window, ICeguiWindowPointerRuntime& runtime);
};

}  // namespace pal4
