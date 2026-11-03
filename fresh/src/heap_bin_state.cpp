#include "pal4/heap_bin_state.h"

namespace pal4 {

std::uint32_t HeapBinState::Initialize(const std::uint32_t heap,
                                       IHeapBinStateRuntime& runtime) {
    const std::uint32_t unsorted = heap + 0x34u;
    for (std::uint32_t index = 0; index < 95u; ++index) {
        const std::uint32_t sentinel = unsorted + index * 8u;
        runtime.Write(sentinel + 12u, sentinel);
        runtime.Write(sentinel + 8u, sentinel);
    }
    runtime.Write(heap + 0x348u, 0u);
    runtime.Write(heap + 0x34cu, 0x40000u);
    runtime.Write(heap + 0x344u, 0x40000u);
    runtime.Write(heap + 0x354u, 0x10000u);
    runtime.Write(heap + 0x360u, runtime.Read(heap + 0x360u) | 1u);
    runtime.Write(heap + 0x2cu, unsorted);
    runtime.Write(heap, (runtime.Read(heap) & 3u) | 0x48u);
    const std::uint32_t page_size = runtime.PageSize();
    runtime.Write(heap + 0x35cu, page_size);
    return page_size;
}

}  // namespace pal4
