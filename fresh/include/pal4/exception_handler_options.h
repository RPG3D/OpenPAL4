#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class ExceptionHandlerOptions final {
public:
    static constexpr std::array<std::uint32_t, 5> kOriginalAddresses{
        0x8a1180u, 0x8a1184u, 0x8a1188u, 0x8e0688u, 0x8a118cu};

    [[nodiscard]] std::uint32_t Set(std::uint32_t first, std::uint32_t second,
                                    std::uint32_t third, std::uint32_t fourth,
                                    std::uint32_t fifth);
    [[nodiscard]] const std::array<std::uint32_t, 5>& Values() const {
        return values_;
    }

private:
    std::array<std::uint32_t, 5> values_{};
};

}  // namespace pal4
