#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class ICeguiImeRuntime {
public:
    virtual ~ICeguiImeRuntime() = default;
    [[nodiscard]] virtual int MultiByteToWideChar(
        std::uint32_t code_page, std::uint32_t flags,
        const std::array<std::uint8_t, 2>& bytes,
        std::uint16_t& output, std::uint32_t output_capacity) = 0;
};

class ICeguiKeyLookupRuntime {
public:
    virtual ~ICeguiKeyLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SearchNode(
        std::uint32_t map_address, std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
};

class CeguiUiHelpers final {
public:
    [[nodiscard]] static std::uint16_t ConvertImeCharacter(
        std::uint8_t first_byte, std::uint16_t initial_wide,
        ICeguiImeRuntime& runtime);
    [[nodiscard]] static std::uint32_t MapVirtualKey(
        std::uint32_t owner, std::uint32_t virtual_key,
        ICeguiKeyLookupRuntime& runtime);
};

}  // namespace pal4
