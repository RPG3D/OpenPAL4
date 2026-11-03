#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ITemperatureWindowRuntime {
public:
    virtual ~ITemperatureWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t object,
                            std::uint32_t layout_name) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view WindowName(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::uint32_t object, std::string_view name) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseNameBuffer() = 0;
    [[nodiscard]] virtual std::uint32_t SetXPosition(
        std::uint32_t window, std::uint32_t metrics_mode,
        float x) = 0;
};

class TemperatureWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x843f10u;

    // IDA 0x4a2360.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name,
        ITemperatureWindowRuntime& runtime);
    // IDA 0x4a23c0.
    [[nodiscard]] static std::uint32_t SetImages(
        std::uint32_t object, ITemperatureWindowRuntime& runtime);
    // IDA 0x4a2580.
    [[nodiscard]] static std::uint32_t SetPosition(
        std::uint32_t object, float position,
        ITemperatureWindowRuntime& runtime);
};

}  // namespace pal4
