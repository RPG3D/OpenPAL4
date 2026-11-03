#include "pal4/inventory_record_memory.h"

namespace pal4 {

std::uint32_t InventoryRecordMemory::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    IInventoryRecordMemoryRuntime& runtime) {
    if (destination == 0u) return 0u;
    runtime.WriteWord(destination, runtime.ReadWord(source));
    runtime.WriteWord(destination + 4u, runtime.ReadWord(source + 4u));
    runtime.WriteWord(destination, runtime.ReadWord(source));
    runtime.WriteWord(destination + 4u, runtime.ReadWord(source + 4u));
    runtime.WriteWord(destination + 8u, runtime.ReadWord(source + 8u));
    return destination;
}

std::uint32_t InventoryRecordMemory::CopyRange(
    const std::uint32_t first, const std::uint32_t last,
    std::uint32_t destination, IInventoryRecordMemoryRuntime& runtime) {
    for (auto source = first; source != last; source += 12u) {
        (void)CopyConstruct(destination, source, runtime);
        destination += 12u;
    }
    return destination;
}

std::uint32_t InventoryRecordMemory::FillConstruct(
    std::uint32_t destination, const std::int32_t count,
    const std::uint32_t source, IInventoryRecordMemoryRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::int32_t index = 0; index < count; ++index) {
        result = CopyConstruct(destination, source, runtime);
        destination += 12u;
    }
    return result;
}

std::uint32_t InventoryRecordMemory::FillAssign(
    const std::uint32_t first, const std::uint32_t last,
    const std::uint32_t source, IInventoryRecordMemoryRuntime& runtime) {
    for (auto destination = first; destination != last;
         destination += 12u) {
        runtime.WriteWord(destination, runtime.ReadWord(source));
        runtime.WriteWord(destination + 4u,
                          runtime.ReadWord(source + 4u));
        runtime.WriteWord(destination + 8u,
                          runtime.ReadWord(source + 8u));
    }
    return last;
}

std::uint32_t InventoryRecordMemory::CopyBackward(
    const std::uint32_t first, std::uint32_t last,
    std::uint32_t destination_end,
    IInventoryRecordMemoryRuntime& runtime) {
    while (last != first) {
        last -= 12u;
        destination_end -= 12u;
        runtime.WriteWord(destination_end, runtime.ReadWord(last));
        runtime.WriteWord(destination_end + 4u,
                          runtime.ReadWord(last + 4u));
        runtime.WriteWord(destination_end + 8u,
                          runtime.ReadWord(last + 8u));
    }
    return destination_end;
}

}  // namespace pal4
