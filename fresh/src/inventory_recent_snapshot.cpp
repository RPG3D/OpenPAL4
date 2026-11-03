#include "pal4/inventory_recent_snapshot.h"

namespace pal4 {

std::uint32_t InventoryRecentSnapshot::Copy(
    std::uint32_t inventory, std::uint32_t destination,
    IInventoryRecentSnapshotRuntime& runtime) {
    runtime.WriteByte(destination, runtime.ReadByte(inventory + 0x6cu));
    const auto begin = runtime.ReadWord(inventory + 0x70u);
    const auto end = runtime.ReadWord(inventory + 0x74u);
    const auto count = begin == 0u ? 0 :
        static_cast<std::int32_t>(end - begin) >> 2;
    const auto bytes = count < 0 ? 0u :
        static_cast<std::uint32_t>(count) * 4u;
    const auto allocated = runtime.Allocate(bytes);
    runtime.WriteWord(destination + 4u, allocated);

    auto source = begin;
    auto output = allocated;
    while (source != end) {
        if (output != 0u)
            runtime.WriteWord(output, runtime.ReadWord(source));
        source += 4u;
        output += 4u;
    }
    runtime.WriteWord(destination + 8u, output);
    runtime.WriteWord(destination + 12u, output);
    return destination;
}

}  // namespace pal4
