#include "pal4/package_name_crc.h"

namespace pal4 {

void PackageNameCrc::InitializeTable() noexcept {
    if (initialized_) return;
    for (std::uint32_t index = 0; index < 256u; ++index) {
        std::uint32_t value = index << 24u;
        for (int bit = 0; bit < 8; ++bit)
            value = (value << 1u) ^
                    ((value & 0x80000000u) ? 0x04c11db7u : 0u);
        table_[index] = value;
    }
    initialized_ = true;
}

std::uint32_t PackageNameCrc::Compute(const std::uint8_t* name) const noexcept {
    if (*name == 0u) return 0u;
    std::uint32_t prefix = 0u;
    const std::uint8_t* cursor = name;
    for (int index = 0; index < 4 && *cursor != 0u; ++index)
        prefix |= std::uint32_t(*cursor++) << (24 - index * 8);

    std::uint32_t value = ~prefix;
    while (*cursor != 0u) {
        value = table_[value >> 24u] ^
                ((value << 8u) | std::uint32_t(*cursor));
        ++cursor;
    }
    return ~value;
}

}  // namespace pal4
