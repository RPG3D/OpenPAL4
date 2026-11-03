#pragma once

#include <cstdint>

namespace pal4 {

class IClickGameWindowLifecycleRuntime {
public:
    virtual ~IClickGameWindowLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessUIEvents(std::uint32_t manager,
                                 bool resume_game) = 0;
    virtual void SetParent(std::uint32_t object, std::uint8_t modal,
                           std::uint32_t parent) = 0;
    virtual void SetModalState(std::uint32_t window,
                               std::uint8_t modal) = 0;
    virtual void SetWidth(std::uint32_t window, float width) = 0;
    [[nodiscard]] virtual std::uint8_t IsVisible(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t Close(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second) = 0;
};

class ClickGameWindowLifecycle final {
public:
    // IDA 0x49f5c0.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IClickGameWindowLifecycleRuntime& runtime);
    // IDA 0x49f5d0.
    static void SetModal(std::uint32_t object, std::uint8_t modal,
                         std::uint32_t parent,
                         IClickGameWindowLifecycleRuntime& runtime);
    // IDA 0x49f620.
    [[nodiscard]] static std::uint32_t UpdateTimer(
        std::uint32_t object, double elapsed,
        IClickGameWindowLifecycleRuntime& runtime);
    // IDA 0x49f660.
    [[nodiscard]] static std::uint32_t UpdateProgress(
        std::uint32_t object, float progress,
        IClickGameWindowLifecycleRuntime& runtime);
    // IDA 0x49f6a0.
    [[nodiscard]] static std::uint8_t OnProgressClick(
        std::uint32_t object, std::uint32_t event,
        IClickGameWindowLifecycleRuntime& runtime);
    // IDA 0x49f6d0.
    [[nodiscard]] static std::uint8_t OnKeyUp(
        std::uint32_t object, std::uint32_t event,
        IClickGameWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
