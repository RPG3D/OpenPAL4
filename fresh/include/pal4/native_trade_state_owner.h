#pragma once
#include "pal4/trade_state.h"
namespace pal4 {
// Four-byte state4 image from5B6CF8..5B6D13. EXE vtable is identity only.
class NativeTradeStateOwner final {
public:
    NativeTradeStateOwner() = default;
    NativeTradeStateOwner(const NativeTradeStateOwner&) = delete;
    NativeTradeStateOwner& operator=(const NativeTradeStateOwner&) = delete;
    std::uint32_t Construct();
    void Destroy() noexcept { image_ = 0u; }
    std::uint32_t Address() const noexcept;
private:
    std::uint32_t image_{};
};
}
