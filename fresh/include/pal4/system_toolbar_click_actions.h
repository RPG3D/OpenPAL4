#pragma once

#include <cstdint>

namespace pal4 {

class ISystemToolbarOperationsRuntime;

class ISystemToolbarClickRuntime {
public:
    virtual ~ISystemToolbarClickRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t name) = 0;
    virtual void ClearAllSelection(std::uint32_t frame) = 0;
    virtual void Close(std::uint32_t frame) = 0;
    virtual void DispatchEvent(std::uint32_t tree, std::uint32_t key,
                               std::uint32_t first,
                               std::uint32_t second) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
};

class SystemToolbarClickActions final {
public:
    static constexpr std::uint32_t kNameSource = 0x8b07d8u;
    static constexpr std::uint32_t kMainEvent = 0x8a80acu;

    [[nodiscard]] static std::uint8_t HandleClick1(
        ISystemToolbarClickRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleClick1(
        ISystemToolbarClickRuntime& runtime,
        ISystemToolbarOperationsRuntime& operations);
    [[nodiscard]] static bool HandleClick2(
        ISystemToolbarClickRuntime& runtime);
    [[nodiscard]] static bool HandleClick2(
        ISystemToolbarClickRuntime& runtime,
        ISystemToolbarOperationsRuntime& operations);
    [[nodiscard]] static std::uint8_t HandleClick3(
        std::uint32_t event, ISystemToolbarClickRuntime& runtime);
};

}  // namespace pal4
