#pragma once

#include <cstdint>

namespace pal4 {

class IMenuWindowActionRuntime {
public:
    virtual ~IMenuWindowActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void HideAndCleanup(std::uint32_t menu) = 0;
    [[nodiscard]] virtual std::uint32_t CallVirtualSlot4(
        std::uint32_t menu, std::uint32_t first,
        std::uint32_t second) = 0;
    virtual void CopyWindowEventArgs(std::uint32_t event) = 0;
    virtual void DestroyWindowEventArgs() = 0;
    virtual void RemoveCurrentHandler(
        std::uint32_t navigation) = 0;
    virtual void ProcessButtonData(std::uint32_t record) = 0;
    [[nodiscard]] virtual float GetYPosition(
        std::uint32_t window, std::uint32_t mode) = 0;
    [[nodiscard]] virtual float GetHeight(
        std::uint32_t window, std::uint32_t mode) = 0;
    virtual void SetHeight(std::uint32_t window,
                           std::uint32_t mode, float height) = 0;
};

class MenuWindowActions final {
public:
    // IDA 0x4b4b30.
    [[nodiscard]] static std::uint8_t HandleClick(
        std::uint32_t menu, IMenuWindowActionRuntime& runtime);
    // IDA 0x4b4b40.
    [[nodiscard]] static std::uint8_t HandleButtonClick(
        std::uint32_t menu, std::uint32_t event,
        IMenuWindowActionRuntime& runtime);
    // IDA 0x4b4c00.
    [[nodiscard]] static std::uint8_t HandleClickEvent(
        std::uint32_t menu, std::uint32_t event,
        IMenuWindowActionRuntime& runtime);
    // IDA 0x4b4c80.
    [[nodiscard]] static std::uint32_t Close(
        std::uint32_t menu, IMenuWindowActionRuntime& runtime);
    // IDA 0x4b4ca0.
    static void AdjustHeight(std::uint32_t menu,
                             IMenuWindowActionRuntime& runtime);
};

}  // namespace pal4
