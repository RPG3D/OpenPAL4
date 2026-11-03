#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

namespace pal4 {

class IRenderWareVector2;

class IWorldMapFlyStartRuntime {
public:
    virtual ~IWorldMapFlyStartRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window,
                                         bool inherited) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::pair<float, float> GetPosition(
        std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t CopyLocationName(
        std::uint32_t record) = 0;
    virtual void ReleaseLocationName(std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructCallback(
        std::uint32_t storage, std::uint32_t frame,
        std::uint32_t function, std::uint32_t name) = 0;
    virtual void Free(std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructTimerEntry(
        std::uint32_t name_address, std::uint32_t interval_bits,
        std::uint32_t callback) = 0;
    virtual void AddTimerEntry(std::uint32_t queue,
                               std::uint32_t entry) = 0;
    virtual void DestroyTimerEntry(std::uint32_t entry) = 0;
    [[nodiscard]] virtual std::uint32_t SequenceImage(
        std::string_view name) = 0;
};

class WorldMapFlyStart final {
public:
    static constexpr std::uint32_t kFlyCallback = 0x484c10u;

    static void Start(std::uint32_t frame,
                      IWorldMapFlyStartRuntime& runtime,
                      IRenderWareVector2& renderware);
};

}  // namespace pal4
