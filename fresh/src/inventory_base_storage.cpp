#include "pal4/inventory_base_storage.h"

namespace pal4 {

namespace {

void InitializeElement(const std::uint32_t element,
                       const std::uint8_t seed,
                       IInventoryBaseStorageRuntime& runtime) {
    runtime.WriteByte(element, seed);
    runtime.WriteWord(element + 4u, 0u);
    runtime.WriteWord(element + 8u, 0u);
    runtime.WriteWord(element + 12u, 0u);
}

void CleanupElement(const std::uint32_t element,
                    IInventoryBaseStorageRuntime& runtime) {
    runtime.FreeMemory(runtime.ReadWord(element + 4u));
    runtime.WriteWord(element + 4u, 0u);
    runtime.WriteWord(element + 8u, 0u);
    runtime.WriteWord(element + 12u, 0u);
}

}  // namespace

std::uint32_t InventoryBaseStorage::Construct(
    const std::uint32_t object, const std::uint8_t element_seed,
    const std::uint8_t tail_seed, IInventoryBaseStorageRuntime& runtime) {
    runtime.WriteWord(object + 4u, 1u);
    runtime.WriteWord(object, kVtable);
    std::uint32_t constructed = 0u;
    try {
        for (; constructed < 5u; ++constructed)
            InitializeElement(object + 8u + constructed * 16u,
                              element_seed, runtime);
    } catch (...) {
        while (constructed != 0u) {
            --constructed;
            CleanupElement(object + 8u + constructed * 16u, runtime);
        }
        throw;
    }
    runtime.WriteWord(object + 112u, 0u);
    runtime.WriteByte(object + 108u, tail_seed);
    runtime.WriteWord(object + 116u, 0u);
    runtime.WriteWord(object + 120u, 0u);
    (void)Reset(object, runtime);
    return object;
}

std::uint32_t InventoryBaseStorage::Reset(
    const std::uint32_t object, IInventoryBaseStorageRuntime& runtime) {
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto element = object + 8u + index * 16u;
        runtime.WriteWord(element + 8u, runtime.ReadWord(element + 4u));
        runtime.WriteWord(object + 88u + index * 4u, 0u);
    }
    const auto old_end = runtime.ReadWord(object + 116u);
    runtime.WriteWord(object + 116u, runtime.ReadWord(object + 112u));
    return old_end;
}

void InventoryBaseStorage::Cleanup(
    const std::uint32_t object, IInventoryBaseStorageRuntime& runtime) {
    (void)Reset(object, runtime);
    runtime.FreeMemory(runtime.ReadWord(object + 112u));
    runtime.WriteWord(object + 112u, 0u);
    runtime.WriteWord(object + 116u, 0u);
    runtime.WriteWord(object + 120u, 0u);
    for (std::uint32_t index = 5u; index != 0u; --index)
        CleanupElement(object + 8u + (index - 1u) * 16u, runtime);
    runtime.WriteWord(object + 4u, 1u);
    runtime.WriteWord(object, kBaseVtable);
}

}  // namespace pal4
