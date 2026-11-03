#include "pal4/trade_window.h"

#include <array>
#include <string>
#include <vector>

namespace pal4 {
namespace {

void ReleaseSharedText(const std::uint32_t address,
                       ITradeWindowRuntime& runtime) {
    const auto data = runtime.ReadWord(address + 4u);
    if (data != 0u) {
        const auto count = runtime.ReadByte(data - 1u);
        if (count == 0u || count == 0xffu) {
            runtime.Free(data - 1u);
        } else {
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(count - 1u));
        }
    }
    for (const auto offset : {4u, 8u, 12u}) {
        runtime.WriteWord(address + offset, 0u);
    }
}


}  // namespace

std::uint32_t TradeWindow::Destroy(const std::uint32_t frame,
                                   ITradeWindowRuntime& runtime) {
    for (const auto base : {0x150u, 0x140u, 0x130u}) {
        runtime.Free(runtime.ReadWord(frame + base + 4u));
        for (const auto offset : {4u, 8u, 12u}) {
            runtime.WriteWord(frame + base + offset, 0u);
        }
    }
    runtime.DestroyNameNode(frame + 0x128u);
    ReleaseSharedText(frame + 0x10cu, runtime);
    ReleaseSharedText(frame + 0xfcu, runtime);
    return runtime.DestroyBase(frame);
}

}  // namespace pal4
