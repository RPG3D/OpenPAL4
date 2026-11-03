#include "pal4/legacy_crc32.h"

namespace pal4 {

std::uint32_t LegacyCrc32::ComputeAndStore(
    const std::uint8_t* data, std::uint32_t length) {
    if (!initialized_) {
        for (std::uint32_t index = 256; index != 0; --index) {
            auto value = index - 1;
            for (int bit = 0; bit < 8; ++bit)
                value = (value >> 1) ^
                        ((value & 1u) ? 0xedb88320u : 0u);
            table_[index - 1] = value;
        }
        initialized_ = true;
    }
    auto result = 0xffffffffu;
    if (data) {
        for (std::uint32_t index = 0; index < length; ++index)
            result = table_[(result ^ data[index]) & 0xffu] ^
                     (result >> 8);
    }
    last_result_ = result;
    return result;
}

bool GameDataRowHashGate::Update(
    std::uint32_t& row_hash, const std::uint8_t* data,
    std::uint32_t length, std::int32_t state, LegacyCrc32& crc) {
    row_hash = crc.ComputeAndStore(data, length);
    return state >= 0 && state < 4;
}

bool GameDataRowHashGate::IsStateAtMostTwo(
    std::uint32_t state) noexcept {
    return state <= 2;
}

}  // namespace pal4
