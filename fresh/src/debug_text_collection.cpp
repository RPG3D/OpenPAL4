#include "pal4/debug_text_collection.h"

#include <cstring>

namespace pal4 {

static_assert(sizeof(DebugTextRecord) == 24u);

std::uint32_t DebugTextCollection::Add(
    const char* text, const std::uint32_t first,
    const std::uint32_t second, const std::uint32_t third,
    IDebugTextRuntime& runtime) {
    if (game_.Read(0x554u) == 0u) {
        return game_.Address();
    }

    DebugTextRecord record;
    runtime.InitializeTemporary(record.text);
    try {
        runtime.AssignTemporary(record.text, text,
                                static_cast<std::uint32_t>(std::strlen(text)));
        record.attributes = {first, second, third};
        runtime.Insert(collection_, record);
    } catch (...) {
        (void)runtime.CleanupTemporary(record.text);
        throw;
    }
    return runtime.CleanupTemporary(record.text);
}

std::uint32_t DebugTextCollection::ClearIfPending(
    IDebugTextRuntime& runtime) {
    const auto count = runtime.GetAndClearProperty(collection_);
    if (count > 0) {
        return runtime.ClearCollection(collection_);
    }
    return static_cast<std::uint32_t>(count);
}

std::uint32_t DebugTextCollection::Draw(
    const std::uint32_t ignored, IDebugTextRuntime& runtime) {
    (void)ignored;
    const auto count = runtime.GetAndClearProperty(collection_);
    if (count <= 0) {
        return static_cast<std::uint32_t>(count);
    }
    if ((globals_.string_guard & 1u) == 0u) {
        globals_.string_guard =
            static_cast<std::uint8_t>(globals_.string_guard | 1u);
        runtime.InitializeStaticString(kStaticStringAddress);
        runtime.RegisterCleanup(kCleanupCallbackAddress);
    }

    std::int32_t bottom = 0;
    for (auto element = runtime.FirstElement(collection_); element != 0u;
         element = runtime.AdvanceElement(collection_)) {
        globals_.top = bottom;
        bottom = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(bottom) + 20u);
        globals_.left = 384;
        globals_.right = 784;
        globals_.bottom = bottom;
        const auto text = runtime.ReadTextPointer(element);
        const auto font = runtime.InitializeFontManager();
        runtime.DrawFontRectangle(font, text, globals_, 1u,
                                  0xff00ff00u, 16u);
    }
    return runtime.ClearCollection(collection_);
}

void DebugTextCollection::CleanupStaticString(IDebugTextRuntime& runtime) {
    runtime.CleanupStaticString(kStaticStringAddress);
}

}  // namespace pal4
