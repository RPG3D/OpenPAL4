#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct NilStringDataHeader {
    std::uint32_t field_00 = 0;
    std::uint32_t field_04 = 0;
    std::uint32_t field_08 = 0;
};

class NilStringDataHeaderAccessors final {
public:
    [[nodiscard]] static std::uint32_t SetField00(
        NilStringDataHeader& record, std::uint32_t value) noexcept;
    [[nodiscard]] static std::uint32_t SetField04(
        NilStringDataHeader& record, std::uint32_t value) noexcept;
    [[nodiscard]] static std::uint32_t SetField08(
        NilStringDataHeader& record, std::uint32_t value) noexcept;
};

class PropertyValueRecord final {
public:
    static constexpr std::size_t kFieldCount = 7;
    using Fields = std::array<std::uint32_t, kFieldCount>;

    [[nodiscard]] static std::uint32_t GetAndClearAt14(
        Fields& fields) noexcept;
    [[nodiscard]] static std::uint32_t GetAndClearAt18(
        Fields& fields) noexcept;
};

}  // namespace pal4
