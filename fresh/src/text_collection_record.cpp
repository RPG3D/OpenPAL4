#include "pal4/text_collection_record.h"

namespace pal4 {

static_assert(sizeof(TextCollectionRecord) == 24u);

std::uint32_t TextCollectionRecordLifecycle::InitializeFromRecord(
    TextCollectionRecord& destination,
    const TextCollectionRecord& source,
    ITextCollectionRecordRuntime& runtime) {
    runtime.AssignText(destination.text, source.text.data,
                       source.text.length);
    destination.attributes[0] = source.attributes[0];
    destination.attributes[1] = source.attributes[1];
    return destination.attributes[1];
}

std::uint32_t TextCollectionRecordLifecycle::Create(
    ITextCollectionRecordRuntime& runtime) {
    const auto address = runtime.Allocate(24u);
    if (address == 0u) {
        return 0u;
    }
    try {
        auto& record = runtime.Access(address);
        runtime.InitializeEmpty(record.text);
        record.attributes[0] = 0u;
        record.attributes[1] = 0u;
    } catch (...) {
        (void)runtime.Free(address);
        throw;
    }
    return address;
}

std::optional<std::uint32_t> TextCollectionRecordLifecycle::Destroy(
    const std::uint32_t address, ITextCollectionRecordRuntime& runtime) {
    if (address == 0u) {
        return std::nullopt;
    }
    runtime.CleanupText(runtime.Access(address).text);
    return runtime.Free(address);
}

}  // namespace pal4
