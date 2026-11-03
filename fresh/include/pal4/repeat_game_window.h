#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class RepeatGameWindowEvent { KeyDown, MouseClick };

class IRepeatGameWindowRuntime {
public:
    virtual ~IRepeatGameWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t object,
                            std::uint32_t layout_name) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view WindowName(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::uint32_t object, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, RepeatGameWindowEvent event,
        std::uint32_t callback) = 0;
    virtual void ReleaseSubscription(std::uint32_t subscription) = 0;
};

class IRepeatGameWindowLifecycleRuntime {
public:
    virtual ~IRepeatGameWindowLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void ProcessUiEvents(std::uint32_t manager, bool hiding) = 0;
    virtual void SetParent(std::uint32_t object, bool visible,
                           std::uint32_t parent) = 0;
    virtual void SetModal(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual bool IsVisible(std::uint32_t object) = 0;
    virtual void SetXPosition(std::uint32_t window, std::uint32_t metrics_mode,
                              float x) = 0;
    [[nodiscard]] virtual float GetXPosition(
        std::uint32_t window, std::uint32_t metrics_mode) = 0;
    [[nodiscard]] virtual std::uint32_t Close(std::uint32_t object) = 0;
};

class RepeatGameWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x843ee4u;
    static constexpr std::uint32_t kCallbackVtable = 0x843edcu;

    // IDA 0x4a1e70.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name,
        IRepeatGameWindowRuntime& runtime);
    // IDA 0x4a2080-0x4a21b0.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IRepeatGameWindowLifecycleRuntime& runtime);
    static void SetVisible(std::uint32_t object, bool visible,
                           std::uint32_t parent,
                           IRepeatGameWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdatePosition(
        std::uint32_t object, double elapsed,
        IRepeatGameWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPosition(
        std::uint32_t object, float progress,
        IRepeatGameWindowLifecycleRuntime& runtime);
    [[nodiscard]] static bool CheckPositionRange(
        std::uint32_t object, IRepeatGameWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnMouseClick(
        std::uint32_t object, std::uint32_t event,
        IRepeatGameWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t OnKeyDown(
        std::uint32_t object, std::uint32_t event,
        IRepeatGameWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
