#include "pal4/minimap_grid_state.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace pal4 {

std::uint32_t MinimapGridState::Cleanup(
    IMinimapGridRuntime& runtime) {
    std::uint32_t result = grid_address_;
    if (grid_address_ != 0u) {
        result = runtime.DestroyPluginObject(grid_address_);
        grid_address_ = 0u;
    }
    flags_ = 0u;
    return result;
}

std::uint32_t MinimapGridState::Initialize(
    const std::uint32_t grid_address, const std::uint32_t flags,
    const std::array<std::uint8_t, 24>& data,
    IMinimapGridRuntime& runtime) {
    (void)Cleanup(runtime);
    grid_address_ = grid_address;
    flags_ = flags;
    data_ = data;
    return grid_address;
}

std::uint32_t MinimapGridState::ReadGridField(
    const std::uint32_t offset, IMinimapGridRuntime& runtime) const {
    return grid_address_ == 0u ? 0u :
        runtime.ReadDword(grid_address_ + offset);
}

std::uint32_t MinimapGridState::PixelBuffer(
    IMinimapGridRuntime& runtime) const {
    return ReadGridField(20u, runtime);
}

std::uint32_t MinimapGridState::Stride(
    IMinimapGridRuntime& runtime) const {
    return ReadGridField(16u, runtime);
}

std::uint32_t MinimapGridState::ReadField12(
    IMinimapGridRuntime& runtime) const {
    return ReadGridField(12u, runtime);
}

std::uint32_t MinimapGridState::Width(
    IMinimapGridRuntime& runtime) const {
    return ReadGridField(4u, runtime);
}

std::uint32_t MinimapGridState::Height(
    IMinimapGridRuntime& runtime) const {
    return ReadGridField(8u, runtime);
}

std::uint8_t MinimapGridState::ApplyRadius(
    const std::int32_t x, const std::int32_t y,
    IMinimapGridEffectsRuntime& runtime) const {
    if (grid_address_ == 0u || x < 0 || y < 0) return 0u;
    const auto width = static_cast<std::int32_t>(Width(runtime));
    if (x >= width) return 0u;
    const auto height = static_cast<std::int32_t>(Height(runtime));
    if (y >= height) return 0u;

    const auto left = std::max(x - 30, 0);
    const auto top = std::max(y - 30, 0);
    const auto right = std::min(x + 30, width);
    const auto bottom = std::min(y + 30, height);
    const auto stride = Stride(runtime);
    for (auto row = top; row < bottom; ++row) {
        const auto row_base = PixelBuffer(runtime) +
            static_cast<std::uint32_t>(row) * stride;
        for (auto column = left; column < right; ++column) {
            const auto dx = column - x;
            const auto dy = row - y;
            const auto distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (distance > 30.0f) continue;
            const auto opacity = distance > 20.0f
                ? runtime.ConvertOpacity(
                      (1.0 - (static_cast<double>(distance) - 20.0) * 0.1) *
                      253.0)
                : 253;
            const auto address = row_base +
                static_cast<std::uint32_t>(column) * 4u + 3u;
            const auto current = runtime.ReadByte(address);
            runtime.WriteByte(address, static_cast<std::uint8_t>(
                std::max(static_cast<std::int32_t>(current), opacity)));
        }
    }
    const auto center = PixelBuffer(runtime) +
        static_cast<std::uint32_t>(y) * stride +
        static_cast<std::uint32_t>(x) * 4u + 3u;
    runtime.WriteByte(center, 0xfeu);
    return 1u;
}

std::uint8_t MinimapGridState::EncodeOpaqueMask(
    std::vector<std::uint32_t>& words,
    IMinimapGridEffectsRuntime& runtime) const {
    if (grid_address_ == 0u) return 0u;
    const auto height = static_cast<std::int32_t>(Height(runtime));
    const auto width = static_cast<std::int32_t>(Width(runtime));
    if (height <= 0) return 1u;
    for (std::int32_t row = 0; row < height; ++row) {
        const auto stride = Stride(runtime);
        auto pixel = PixelBuffer(runtime) +
            static_cast<std::uint32_t>(row) * stride;
        std::int32_t column = 0;
        do {
            std::uint32_t word = 0u;
            std::uint32_t bit = 0u;
            do {
                if (runtime.ReadByte(pixel + 3u) >= 0xfeu) {
                    word |= 1u << bit;
                }
                pixel += 4u;
                ++column;
                if (column >= width) break;
                ++bit;
            } while (bit < 32u);
            words.push_back(word);
        } while (column < width);
    }
    return 1u;
}

std::uint8_t MinimapGridState::ApplyOpaqueMask(
    const std::vector<std::uint32_t>& words,
    IMinimapGridEffectsRuntime& runtime) const {
    if (grid_address_ == 0u) return 0u;
    const auto height = static_cast<std::int32_t>(Height(runtime));
    const auto width = static_cast<std::int32_t>(Width(runtime));
    std::size_t next_word = 0u;
    for (std::int32_t row = 0; row < height; ++row) {
        std::int32_t column = 0;
        while (column < width) {
            if (next_word == words.size()) return 0u;
            const auto word = words[next_word++];
            for (std::uint32_t bit = 0u; bit < 32u && column < width;
                 ++bit, ++column) {
                if ((word & (1u << bit)) != 0u) {
                    (void)ApplyRadius(column, row, runtime);
                }
            }
        }
    }
    return 1u;
}

const std::array<float, 3>& MinimapGridState::WorldToGridCoordinates(
    std::array<float, 2>& output,
    const std::array<float, 3>& position) const {
    float origin_x;
    float origin_z;
    std::memcpy(&origin_x, data_.data() + 12u, sizeof(origin_x));
    std::memcpy(&origin_z, data_.data() + 20u, sizeof(origin_z));
    const auto scale = static_cast<std::int32_t>(flags_);
    output[0] = static_cast<float>(
        (static_cast<double>(position[0]) - origin_x) / scale);
    output[1] = static_cast<float>(
        (static_cast<double>(position[2]) - origin_z) / scale);
    return position;
}

}  // namespace pal4
