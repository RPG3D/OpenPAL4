#include "pal4/trade_window_item_snapshot.h"

namespace pal4 {

std::uint32_t TradeWindowItemSnapshot::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    ICeguiTextRecordRuntime& runtime) {
    runtime.CopyBytes(destination, source, 0x90u);
    runtime.WriteDword(destination + 0x90u,
                       runtime.ReadDword(source + 0x90u));
    runtime.WriteDword(destination + 0x94u,
                       runtime.ReadDword(source + 0x94u));
    for (std::uint32_t index = 0u; index < kTextCount; ++index) {
        const auto offset = kTextOffset + index * CeguiTextRecord::kBytes;
        CeguiTextRecord::Copy(destination + offset,
                              source + offset, runtime);
    }
    for (std::uint32_t offset = 0x168u; offset < 0x178u;
         offset += 4u)
        runtime.WriteDword(destination + offset,
                           runtime.ReadDword(source + offset));
    runtime.CopyBytes(destination + 0x178u, source + 0x178u, 0x18u);
    return destination;
}

}  // namespace pal4
