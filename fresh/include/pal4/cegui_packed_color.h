#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class CeguiPackedColor final {
public:
    [[nodiscard]] const std::array<std::uint8_t, 4>& Bytes() const noexcept {
        return bytes_;
    }

    [[nodiscard]] CeguiPackedColor& SetAllBytesFF() noexcept;

    class IFloatToInt {
    public:
        virtual ~IFloatToInt() = default;
        [[nodiscard]] virtual std::int32_t Convert(double value) = 0;
    };

    [[nodiscard]] std::int32_t SetFromFloats(
        float first, float second, float third, float fourth,
        IFloatToInt& runtime);

private:
    std::array<std::uint8_t, 4> bytes_{};
};

class CeguiHostFloatToInt final : public CeguiPackedColor::IFloatToInt {
public:
    [[nodiscard]] std::int32_t Convert(double value) override;
};

}  // namespace pal4
