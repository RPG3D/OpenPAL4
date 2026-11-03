#pragma once

namespace pal4 {

class IntroductionWindowRegistration final {
public:
    [[nodiscard]] static const char* RegisterName(
        const char*& destination) noexcept;
    [[nodiscard]] static const char* RegisterNameThunk(
        const char*& destination) noexcept;
};

}  // namespace pal4
