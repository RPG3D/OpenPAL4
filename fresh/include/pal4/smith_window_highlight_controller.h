#pragma once

#include <cstdint>

namespace pal4 {

class ISmithWindowHighlightRuntime {
public:
    virtual ~ISmithWindowHighlightRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window,
                                         bool recursive) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
};

class SmithWindowHighlightController final {
public:
    // IDA 0x49e880.
    [[nodiscard]] static std::uint8_t OnAutoClose(
        std::uint32_t object, ISmithWindowHighlightRuntime& runtime);
    // IDA 0x49e890.
    static void SetHighlighted(
        std::uint32_t object, std::uint8_t highlighted,
        ISmithWindowHighlightRuntime& runtime);
};

}  // namespace pal4
