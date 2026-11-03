#include "pal4/archive_text_reader.h"

#include <array>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace pal4 {

const char* ArchiveTextReadRuntimeStub::EmptyData() { return ""; }
bool ArchiveTextReadRuntimeStub::AllocateText(StringBufferState& text,
                                              std::uint32_t length,
                                              std::uint8_t) {
    if (length == 0u) text = {};
    return false;
}
void ArchiveTextReadRuntimeStub::SetErrorFlag(StringBufferState& text,
                                              std::uint8_t) { text = {}; }
void ArchiveTextReadRuntimeStub::Reallocate(StringBufferState&,
                                             std::uint32_t) {}
void ArchiveTextReadRuntimeStub::SetTextSize(StringBufferState& text,
                                             std::uint32_t length) {
    text.length = length;
}
std::uint8_t ArchiveTextReadRuntimeStub::ReadByte(std::uint32_t) { return 0u; }
void ArchiveTextReadRuntimeStub::WriteByte(std::uint32_t,
                                           std::uint8_t) {}
void ArchiveTextReadRuntimeStub::Copy(std::uint32_t, const char*,
                                      std::uint32_t) {}

std::uint8_t ArchiveTextReader::Read(
    const ArchiveStreamAdapterState& stream, StringBufferState& destination) {
    std::int32_t signed_length = 0;
    if ((adapter_.Read(stream, &signed_length, 4u) & 0xffu) == 0u)
        return 0u;
    // Native code treats negative lengths as short buffers and would overrun
    // its stack frame. Reject malformed input before the second read.
    if (signed_length < 0) return 0u;
    const auto length = static_cast<std::uint32_t>(signed_length);

    if (length < 512u) {
        std::array<char, 512> bytes{};
        const char* source = runtime_.EmptyData();
        if (length != 0u) {
            if ((adapter_.Read(stream, bytes.data(), length) & 0xffu) == 0u)
                return 0u;
            source = bytes.data();
        }
        const auto text_length = static_cast<std::uint32_t>(
            std::strlen(source));
        if (runtime_.AllocateText(destination, text_length, 1u)) {
            runtime_.Copy(destination.data, source, text_length);
            destination.length = text_length;
            runtime_.WriteByte(destination.data + text_length, 0u);
        }
        return 1u;
    }

    std::vector<char> bytes(static_cast<std::size_t>(length) + 1u, '\0');
    if ((adapter_.Read(stream, bytes.data(), length) & 0xffu) == 0u)
        return 0u;
    const auto text_length = static_cast<std::uint32_t>(
        std::strlen(bytes.data()));
    if (text_length > StringBuffer::MaxSize())
        throw std::length_error("archive text exceeds string max size");

    if (destination.data != 0u) {
        const auto reference = runtime_.ReadByte(destination.data - 1u);
        if (reference != 0u && reference != 0xffu) {
            if (text_length != 0u) {
                runtime_.Reallocate(destination, text_length);
            } else {
                runtime_.WriteByte(destination.data - 1u,
                                   static_cast<std::uint8_t>(reference - 1u));
                runtime_.SetErrorFlag(destination, 0u);
                return 1u;
            }
        } else if (text_length == 0u) {
            runtime_.SetErrorFlag(destination, 1u);
            return 1u;
        } else if (destination.capacity > 31u ||
                   destination.capacity < text_length) {
            runtime_.SetErrorFlag(destination, 1u);
            runtime_.Reallocate(destination, text_length);
        }
    } else if (text_length == 0u) {
        runtime_.SetErrorFlag(destination, 1u);
        return 1u;
    } else if (destination.capacity > 31u ||
               destination.capacity < text_length) {
        runtime_.SetErrorFlag(destination, 1u);
        runtime_.Reallocate(destination, text_length);
    }

    runtime_.Copy(destination.data, bytes.data(), text_length);
    runtime_.SetTextSize(destination, text_length);
    return 1u;
}

}  // namespace pal4
