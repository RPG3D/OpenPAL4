#pragma once

#include "pal4/save_ui_state.h"

#include <cstdint>

namespace pal4 {

// Owns the original four-byte PALIV_GAME_SAVE state image.
class NativeSaveUiStateOwner final : private ISaveUiConstructionRuntime {
public:
    [[nodiscard]] std::uint32_t Construct();
    void Destroy() noexcept;
    [[nodiscard]] std::uint32_t Address() const noexcept;

private:
    void WriteWord(std::uint32_t address, std::uint32_t value) override;

    std::uint32_t image_{};
};

}  // namespace pal4
