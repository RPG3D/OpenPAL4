#pragma once

#include "pal4/system_menu_state.h"

namespace pal4 {

// Four-byte PALIV_SYSTEM image from Game_InitializeComponents (5B6D19..34).
// The EXE vtable is an identity tag; host dispatch uses SystemMenuState.
class NativeSystemMenuStateOwner final {
public:
    NativeSystemMenuStateOwner() = default;
    NativeSystemMenuStateOwner(const NativeSystemMenuStateOwner&) = delete;
    NativeSystemMenuStateOwner& operator=(const NativeSystemMenuStateOwner&) = delete;
    [[nodiscard]] std::uint32_t Construct();
    void Destroy() noexcept { image_ = 0u; }
    [[nodiscard]] std::uint32_t Address() const noexcept;

private:
    std::uint32_t image_{};
};

}  // namespace pal4
