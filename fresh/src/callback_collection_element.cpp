#include "pal4/callback_collection_element.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kElementSize = 0x10;
constexpr std::uint32_t kElementPayloadOffset = 0x08;
constexpr std::uint32_t kElementFactoryTableOffset = 0x0c;

}  // namespace

std::uint32_t CallbackCollectionState::Read(
    const std::size_t offset) const noexcept {
    return fields[offset / sizeof(std::uint32_t)];
}

void CallbackCollectionState::Write(const std::size_t offset,
                                    const std::uint32_t value) noexcept {
    fields[offset / sizeof(std::uint32_t)] = value;
}

bool CallbackCollectionElementInitializer::AllocateAndInitialize(
    CallbackCollectionState& collection, const std::uint32_t self_address,
    ICallbackCollectionRuntime& runtime) {
    collection.Write(0x0c, 0);
    const auto element = runtime.Allocate(kElementSize);
    if (element == 0) {
        collection.Write(0x0c, 1);
        return false;
    }

    runtime.WriteDword(element, 0);
    runtime.WriteDword(element + 4, 0);
    runtime.WriteDword(element + kElementFactoryTableOffset, self_address);

    std::uint32_t payload = 0;
    try {
        payload = runtime.InvokeFactory(self_address);
    } catch (...) {
        runtime.Free(element);
        throw;
    }
    runtime.WriteDword(element + kElementPayloadOffset, payload);

    const auto sentinel = collection.Read(0x20);
    const auto first = runtime.ReadDword(sentinel + 4);
    runtime.WriteDword(element + 4, first);
    const auto previous = runtime.ReadDword(first);
    runtime.WriteDword(element, previous);
    const auto ecx_after_insert = runtime.InvokeInsert(
        collection.Read(0x08), previous, payload, sentinel);

    runtime.WriteDword(runtime.ReadDword(ecx_after_insert + 4), element);
    runtime.WriteDword(sentinel + 4, element);

    collection.Write(0x18, collection.Read(0x18) + 1U);
    collection.Write(0x14, collection.Read(0x14) + 1U);
    collection.Write(0x20, element);
    collection.Write(0x10, 0);
    return true;
}

}  // namespace pal4
