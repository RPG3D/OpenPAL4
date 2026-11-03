#pragma once

#include <cstdint>

namespace pal4 {

class IWorldMapFlyStartRuntime;
class IRenderWareVector2;

class IWorldMapClickRuntime {
public:
    virtual ~IWorldMapClickRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window,
                                         bool inherited) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t TriggerText(
        std::uint32_t collection, std::uint32_t trigger) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateCommonDialog(
        std::uint32_t manager) = 0;
    virtual void ShowDialog(std::uint32_t dialog, std::uint32_t text,
                            std::uint32_t x_bits, std::uint32_t y_bits,
                            std::uint32_t mode, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void ReleaseCallback(std::uint32_t callback,
                                 std::uint32_t flags) = 0;
};

class IWorldMapDialogResultRuntime {
public:
    virtual ~IWorldMapDialogResultRuntime() = default;
    virtual void StartFly(std::uint32_t frame) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class WorldMapClick final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843cc8u;
    static constexpr std::uint32_t kOnDialogResult = 0x484db0u;
    static constexpr std::uint32_t kCenterBits = 0x3f000000u;

    [[nodiscard]] static std::uint8_t OnLocation(
        std::uint32_t frame, std::uint32_t event,
        IWorldMapClickRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnDialogResult(
        std::uint32_t frame, std::uint32_t result,
        IWorldMapDialogResultRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnDialogResult(
        std::uint32_t frame, std::uint32_t result,
        IWorldMapFlyStartRuntime& runtime,
        IRenderWareVector2& renderware);
};

}  // namespace pal4
