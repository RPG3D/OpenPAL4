#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class ClickGameWindowEvent { KeyUp, MouseClick };

class IClickGameWindowRuntime {
public:
    virtual ~IClickGameWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t object,
                            std::uint32_t layout_name) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view WindowName(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::uint32_t object, std::string_view name) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, ClickGameWindowEvent event,
        std::uint32_t callback) = 0;
    virtual void ReleaseSubscription(std::uint32_t subscription) = 0;
};

class ClickGameWindow final {
public:
    // IDA 0x49f3b0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name,
        IClickGameWindowRuntime& runtime);
};

}  // namespace pal4
