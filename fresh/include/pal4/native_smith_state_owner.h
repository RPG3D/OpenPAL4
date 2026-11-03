#pragma once
#include "pal4/smith_state.h"
namespace pal4 {
// Original5B6CD9..F4 allocates four bytes and stores849468 before AddEntry.
// Identity only: host dispatch must not invoke the EXE vtable.
class NativeSmithStateOwner final {
public:
    NativeSmithStateOwner()=default;
    NativeSmithStateOwner(const NativeSmithStateOwner&)=delete;
    NativeSmithStateOwner& operator=(const NativeSmithStateOwner&)=delete;
    std::uint32_t Construct();
    void Destroy()noexcept{image_=0u;}
    std::uint32_t Address()const noexcept;
private:
    std::uint32_t image_{};
};
}
