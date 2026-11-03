#pragma once

#include "pal4/jigsaw_window_input.h"
#include <windows.h>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pal4 {
class NativeGameDatabase;
struct NativePuzzleElement {
    std::uint32_t property{};
    std::array<std::int32_t,2> origin{}, extent{};
    std::array<std::uint32_t,3> images{};
};

// Owns the shared grid's element/anchor buffers and DLL String lifetimes.
// Borrows live CEGUI images/fonts and GameDB; retire before those services.
class NativePuzzleBoard final {
public:
    // FontManager owns both the original static font and its referenced atlas.
    static std::uint32_t LoadOriginalFont(HMODULE);
    NativePuzzleBoard(HMODULE, const NativeGameDatabase&);
    ~NativePuzzleBoard();
    NativePuzzleBoard(const NativePuzzleBoard&) = delete;
    NativePuzzleBoard& operator=(const NativePuzzleBoard&) = delete;
    void Initialize(std::int32_t type);
    std::uint32_t Pointer(const std::array<float,2>& origin, const std::array<float,2>& point);
    std::uint32_t Move(JigsawDirection);
    void Confirm();
    void ResetSelection();
    bool HasActiveSelection() const;
    bool Complete();
    void Render(const std::array<float,2>& origin, float depth);
    std::int32_t Counter() const;
    void SetCounter(std::int32_t);
    bool UsesMoveBudget() const;
    const std::string& TitleUtf8() const;
    std::vector<NativePuzzleElement> Elements() const;
    std::size_t OwnedStrings() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pal4
