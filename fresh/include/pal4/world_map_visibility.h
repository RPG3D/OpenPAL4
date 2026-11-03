#pragma once

#include <cstdint>

namespace pal4 {

class IWorldMapVisibilityRuntime {
public:
    virtual ~IWorldMapVisibilityRuntime() = default;
    virtual void SetParent(std::uint32_t frame, std::uint8_t visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetCursor(std::uint32_t slot, std::uint32_t cursor) = 0;
    virtual void InitializeButtons(std::uint32_t frame) = 0;
    virtual void SetModal(std::uint32_t window, bool modal) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t CopyArenaName(
        std::uint32_t source, std::uint8_t initial_flag) = 0;
    [[nodiscard]] virtual bool NamesEqualIgnoreCase(
        std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t GetPosition(
        std::uint32_t window) = 0;
    virtual void SetPosition(std::uint32_t window,
                             std::uint32_t position) = 0;
    virtual void RefreshMouse(std::uint32_t mouse) = 0;
    virtual void ReleaseArenaName(std::uint32_t name) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class WorldMapVisibility final {
public:
    static constexpr std::uint32_t kDefaultCursor = 0x8b1c70u;
    static constexpr std::uint32_t kLocationCount = 21u;

    static void Update(std::uint32_t frame, std::uint8_t visible,
                       std::uint32_t parent,
                       IWorldMapVisibilityRuntime& runtime);
};

}  // namespace pal4
