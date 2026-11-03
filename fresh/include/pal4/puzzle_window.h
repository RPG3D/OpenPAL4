#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class PuzzleWindowEvent { RenderingEnded, MouseButtonUp, Clicked };

class IPuzzleWindowRuntime {
public:
    virtual ~IPuzzleWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t object,
                            std::uint32_t layout_name) = 0;
    virtual void InitializePuzzle(std::uint32_t puzzle) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void SetString(std::uint32_t address,
                           std::string_view text) = 0;
    [[nodiscard]] virtual std::string_view WindowName(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::uint32_t object, std::string_view name) = 0;
    virtual void SetStaticTextFormatting(
        std::uint32_t window, std::uint32_t horizontal,
        std::uint32_t vertical) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, PuzzleWindowEvent event,
        std::uint32_t callback) = 0;
    virtual void ReleaseSubscription(std::uint32_t subscription) = 0;
};

class PuzzleWindow final {
public:
    // IDA 0x4a0b80.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name,
        IPuzzleWindowRuntime& runtime);
};

}  // namespace pal4
