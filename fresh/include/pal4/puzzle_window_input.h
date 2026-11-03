#pragma once

#include "pal4/jigsaw_window_input.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IPuzzleWindowInputRuntime {
public:
    virtual ~IPuzzleWindowInputRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Renderer() = 0;
    [[nodiscard]] virtual std::array<float, 4> GetBoardRect(
        std::uint32_t board) = 0;
    [[nodiscard]] virtual std::array<float, 2> ReadPoint(
        std::uint32_t address) = 0;
    virtual void RenderPuzzle(std::uint32_t puzzle,
                              const std::array<float, 2>& origin,
                              std::uint32_t renderer) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessMouse(
        std::uint32_t puzzle, const std::array<float, 2>& origin,
        const std::array<float, 2>& point) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessDirection(
        std::uint32_t puzzle, JigsawDirection direction) = 0;
    virtual void ResetSelection(std::uint32_t puzzle) = 0;
    virtual void ConfirmSelection(std::uint32_t puzzle) = 0;
    virtual void Redraw(std::uint32_t board) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateDisplay(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual bool IsComplete(std::uint32_t puzzle) = 0;
    virtual void ShowResult(std::uint32_t frame,
                            std::uint32_t success) = 0;
    virtual void PlaySound(std::string_view file,
                           std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t Close(
        std::uint32_t frame, std::uint32_t first,
        std::uint32_t second) = 0;
};

class PuzzleWindowInput final {
public:
    // IDA 0x4a1250.
    [[nodiscard]] static std::uint8_t OnRender(
        std::uint32_t frame, IPuzzleWindowInputRuntime& runtime);
    // IDA 0x4a12b0.
    [[nodiscard]] static std::uint8_t OnMouseButtonUp(
        std::uint32_t frame, std::uint32_t event,
        IPuzzleWindowInputRuntime& runtime);
    // IDA 0x4a1380, keyboard subobject at frame +0x28.
    [[nodiscard]] static std::uint8_t OnKeyUp(
        std::uint32_t handler, std::uint32_t event,
        IPuzzleWindowInputRuntime& runtime);
    // IDA 0x4a14a0.
    [[nodiscard]] static std::uint8_t OnCloseClick(
        std::uint32_t frame, IPuzzleWindowInputRuntime& runtime);
};

}  // namespace pal4
