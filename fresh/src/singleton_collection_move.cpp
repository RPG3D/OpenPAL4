#include "pal4/singleton_collection_move.h"

namespace pal4 {
namespace {

std::uint32_t TextOrEmpty(const std::uint32_t pointer) {
    return pointer != 0u
        ? pointer : SingletonCollectionMove::kEmptyTextAddress;
}

void MoveOwnText(const std::uint32_t element,
                 const std::uint32_t original_length,
                 ISingletonCollectionMoveRuntime& runtime) {
    if (runtime.ReadDword(element + 8u) < original_length) {
        runtime.ThrowRange();
    }
    runtime.DetachAndReset(element);
    const std::uint32_t remaining =
        runtime.ReadDword(element + 8u) - original_length;
    std::uint32_t removed = 0xffffffffu;
    if (remaining < 0xffffffffu) {
        removed = remaining;
    }
    if (remaining == 0xffffffffu || removed != 0u) {
        const std::uint32_t data = runtime.ReadDword(element + 4u);
        runtime.Copy(data + original_length,
                     data + original_length + removed,
                     remaining - removed);
        const std::uint32_t new_length =
            runtime.ReadDword(element + 8u) - removed;
        if (runtime.AllocateText(element, new_length, 0u)) {
            runtime.SetTextSize(element, new_length);
        }
    }
    runtime.DetachAndReset(element);
}

void MoveOtherText(const std::uint32_t destination,
                   const std::uint32_t source,
                   const std::uint32_t source_length,
                   ISingletonCollectionMoveRuntime& runtime) {
    std::uint32_t source_text = 0u;
    if (source_length != 0u) {
        source_text = TextOrEmpty(runtime.ReadDword(source + 4u));
    }
    if (source_length != 0u &&
        runtime.ReadByte(source_text - 1u) < 0xfeu) {
        const std::uint32_t previous = runtime.ReadDword(destination + 4u);
        if (previous != 0u) {
            const std::uint32_t previous_header = previous - 1u;
            const std::uint8_t reference =
                runtime.ReadByte(previous_header);
            if (reference == 0u || reference == 0xffu) {
                runtime.Free(previous_header);
            } else {
                runtime.WriteByte(previous_header,
                    static_cast<std::uint8_t>(reference - 1u));
            }
        }
        runtime.WriteDword(destination + 4u, 0u);
        runtime.WriteDword(destination + 8u, 0u);
        runtime.WriteDword(destination + 12u, 0u);
        runtime.WriteDword(destination + 4u, source_text);
        runtime.WriteDword(destination + 8u,
                           runtime.ReadDword(source + 8u));
        runtime.WriteDword(destination + 12u,
                           runtime.ReadDword(source + 12u));
        runtime.WriteByte(source_text - 1u,
            static_cast<std::uint8_t>(
                runtime.ReadByte(source_text - 1u) + 1u));
        return;
    }

    if (runtime.AllocateText(destination, source_length, 1u)) {
        const std::uint32_t source_data = TextOrEmpty(
            runtime.ReadDword(source + 4u));
        const std::uint32_t destination_data =
            runtime.ReadDword(destination + 4u);
        runtime.Copy(destination_data, source_data, source_length);
        runtime.WriteDword(destination + 8u, source_length);
        runtime.WriteByte(destination_data + source_length, 0u);
    }
}

void CopyElementWithLength(const std::uint32_t destination,
                           const std::uint32_t source,
                           const std::uint32_t source_length,
                           ISingletonCollectionMoveRuntime& runtime) {
    if (source == destination) {
        MoveOwnText(destination, source_length, runtime);
    } else {
        MoveOtherText(destination, source, source_length, runtime);
    }
    constexpr std::uint32_t offsets[]{16u, 20u, 24u};
    for (const std::uint32_t offset : offsets) {
        runtime.WriteDword(destination + offset,
                           runtime.ReadDword(source + offset));
    }
}

}  // namespace

std::uint32_t SingletonCollectionMove::Backward(
    const std::uint32_t begin, const std::uint32_t end,
    std::uint32_t destination_end,
    ISingletonCollectionMoveRuntime& runtime) {
    if (begin == end) {
        return destination_end;
    }

    std::uint32_t source_end = end;
    do {
        const std::uint32_t source = source_end - kElementBytes;
        const std::uint32_t destination =
            destination_end - kElementBytes;
        const std::uint32_t source_length =
            runtime.ReadDword(source + 8u);

        CopyElementWithLength(destination, source, source_length, runtime);
        source_end = source;
        destination_end = destination;
    } while (source_end != begin);
    return destination_end;
}

std::uint32_t SingletonCollectionMove::CopyElement(
    const std::uint32_t destination, const std::uint32_t source,
    ISingletonCollectionMoveRuntime& runtime) {
    const std::uint32_t source_length =
        runtime.ReadDword(source + 8u);
    CopyElementWithLength(destination, source, source_length, runtime);
    return destination;
}

std::uint32_t SingletonCollectionMove::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    ISingletonCollectionMoveRuntime& runtime) {
    runtime.WriteByte(destination, runtime.ReadByte(source));
    runtime.WriteDword(destination + 4u, 0u);
    runtime.WriteDword(destination + 8u, 0u);
    runtime.WriteDword(destination + 12u, 0u);
    const std::uint32_t source_length = runtime.ReadDword(source + 8u);
    CopyElementWithLength(destination, source, source_length, runtime);
    return destination;
}

}  // namespace pal4
