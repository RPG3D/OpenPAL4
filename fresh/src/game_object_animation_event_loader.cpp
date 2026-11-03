#include "pal4/game_object_animation_event_loader.h"

#include <algorithm>
#include <string>

namespace pal4 {

bool GameObjectAnimationEventLoader::Load(
    const std::uint32_t record, IGameObjectAnimationEventRuntime& runtime) {
    if (record == 0) return false;

    std::string stem(runtime.RecordBasePath(record));
    stem += runtime.RecordName(record);
    const auto stream = runtime.OpenStream(2, 1, stem + ".amf");
    if (stream == 0) return false;

    runtime.ClearEvents(record);
    std::uint32_t header_length = 0;
    runtime.ReadStream(stream, std::as_writable_bytes(std::span{&header_length, 1}));
    std::array<std::byte, 256> header{};
    if (header_length < header.size()) {
        runtime.ReadStream(stream, std::span(header.data(), header_length));
        const auto terminator = std::find(header.begin(), header.end(), std::byte{});
        const std::string_view header_text(
            reinterpret_cast<const char*>(header.data()),
            static_cast<std::size_t>(terminator - header.begin()));
        if (header_text == kSignature) {
            std::int32_t count = 0;
            runtime.ReadStream(stream, std::as_writable_bytes(std::span{&count, 1}));
            for (std::int32_t index = 0; index < count; ++index) {
                AnimationEventData event{};
                (void)GameObjectAnimationEventData::Reset(event);
                runtime.ReadStream(stream, event);
                (void)GameObjectAnimationEventData::NormalizeTime(event);
                runtime.InsertEvent(record, event);
            }
        }
    } else {
        // The original overflows its 256-byte stack header for this input.
        std::array<std::byte, 256> chunk{};
        for (auto remaining = header_length; remaining != 0;) {
            const auto length = std::min<std::uint32_t>(
                remaining, static_cast<std::uint32_t>(chunk.size()));
            runtime.ReadStream(stream, std::span(chunk.data(), length));
            remaining -= length;
        }
    }

    runtime.CloseStream(stream, 0);
    (void)runtime.LoadSidecar(record, stem + ".amd");
    return true;
}

}  // namespace pal4
