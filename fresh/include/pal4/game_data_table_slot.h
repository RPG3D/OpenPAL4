#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class GameDataTableSlot final {
public:
    // IDA 0x4eee90: initialize five consecutive DWORDs to zero.
    [[nodiscard]] GameDataTableSlot& Initialize() noexcept;

    std::array<std::uint32_t, 5> words{};
};

static_assert(sizeof(GameDataTableSlot) == 20);

}  // namespace pal4
