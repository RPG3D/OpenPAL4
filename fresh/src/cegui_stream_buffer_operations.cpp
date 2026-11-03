#include "pal4/cegui_stream_buffer_operations.h"

#include <algorithm>

namespace pal4 {

std::uint32_t CeguiStreamBufferRuntimeStub::ReadDword(
    const std::uint32_t address) { return words_.at(address); }
void CeguiStreamBufferRuntimeStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}
std::uint8_t CeguiStreamBufferRuntimeStub::ReadByte(
    const std::uint32_t address) { return bytes_.at(address); }
void CeguiStreamBufferRuntimeStub::WriteByte(
    const std::uint32_t address, const std::uint8_t value) {
    bytes_[address] = value;
}
void CeguiStreamBufferRuntimeStub::CopyBytes(
    const std::uint32_t destination, const std::uint32_t source,
    const std::uint32_t bytes) {
    for (std::uint32_t index = 0u; index < bytes; ++index) {
        WriteByte(destination + index, ReadByte(source + index));
    }
}
std::int32_t CeguiStreamBufferRuntimeStub::Underflow(
    std::uint32_t /*streambuf*/) { return -1; }
std::int32_t CeguiStreamBufferRuntimeStub::ApplyUnderflowResult(
    std::uint32_t /*streambuf*/, const std::int32_t value) { return value; }
std::int32_t CeguiStreamBufferRuntimeStub::Uflow(
    std::uint32_t /*streambuf*/) { return -1; }
std::int32_t CeguiStreamBufferRuntimeStub::Overflow(
    std::uint32_t /*streambuf*/, std::uint8_t /*value*/) { return -1; }
std::uint32_t CeguiStreamBufferRuntimeStub::FposLocale() { return 0u; }

std::int32_t CeguiStreamBufferOperations::ReadChar(
    const std::uint32_t streambuf, ICeguiStreamBufferRuntime& runtime) {
    const auto available = runtime.Underflow(streambuf);
    if (available == -1) {
        return -1;
    }
    const auto count_address = runtime.ReadDword(streambuf + 44u);
    const auto count = runtime.ReadDword(count_address);
    runtime.WriteDword(count_address, count - 1u);
    const auto cursor_address = runtime.ReadDword(streambuf + 28u);
    const auto cursor = runtime.ReadDword(cursor_address);
    runtime.WriteDword(cursor_address, cursor + 1u);
    return runtime.ReadByte(cursor);
}

std::uint32_t CeguiStreamBufferOperations::ReadBytes(
    const std::uint32_t streambuf, std::uint32_t destination,
    const std::int32_t bytes, ICeguiStreamBufferRuntime& runtime) {
    if (bytes <= 0) {
        return 0u;
    }
    auto remaining = static_cast<std::uint32_t>(bytes);
    std::uint32_t transferred = 0u;
    while (remaining != 0u) {
        const auto cursor_address = runtime.ReadDword(streambuf + 28u);
        const auto cursor = runtime.ReadDword(cursor_address);
        if (cursor != 0u) {
            const auto count_address = runtime.ReadDword(streambuf + 44u);
            const auto count = static_cast<std::int32_t>(
                runtime.ReadDword(count_address));
            if (count > 0) {
                const auto chunk = std::min(
                    remaining, static_cast<std::uint32_t>(count));
                runtime.CopyBytes(destination, cursor, chunk);
                destination += chunk;
                transferred += chunk;
                remaining -= chunk;
                runtime.WriteDword(count_address,
                                   static_cast<std::uint32_t>(count) - chunk);
                runtime.WriteDword(cursor_address, cursor + chunk);
                continue;
            }
        }
        const auto next = runtime.Uflow(streambuf);
        if (next == -1) {
            return transferred;
        }
        runtime.WriteByte(destination++, static_cast<std::uint8_t>(next));
        ++transferred;
        --remaining;
    }
    return transferred;
}

std::uint32_t CeguiStreamBufferOperations::WriteBytes(
    const std::uint32_t streambuf, std::uint32_t source,
    const std::int32_t bytes, ICeguiStreamBufferRuntime& runtime) {
    if (bytes <= 0) {
        return 0u;
    }
    auto remaining = static_cast<std::uint32_t>(bytes);
    std::uint32_t transferred = 0u;
    while (remaining != 0u) {
        const auto cursor_address = runtime.ReadDword(streambuf + 32u);
        const auto cursor = runtime.ReadDword(cursor_address);
        if (cursor != 0u) {
            const auto count_address = runtime.ReadDword(streambuf + 48u);
            const auto count = static_cast<std::int32_t>(
                runtime.ReadDword(count_address));
            if (count > 0) {
                const auto chunk = std::min(
                    remaining, static_cast<std::uint32_t>(count));
                runtime.CopyBytes(cursor, source, chunk);
                source += chunk;
                transferred += chunk;
                remaining -= chunk;
                runtime.WriteDword(count_address,
                                   static_cast<std::uint32_t>(count) - chunk);
                runtime.WriteDword(cursor_address, cursor + chunk);
                continue;
            }
        }
        const auto value = runtime.ReadByte(source);
        if (runtime.Overflow(streambuf, value) == -1) {
            return transferred;
        }
        ++source;
        ++transferred;
        --remaining;
    }
    return transferred;
}

std::uint32_t CeguiStreamBufferOperations::WriteFailurePosition(
    const std::uint32_t output, ICeguiStreamBufferRuntime& runtime) {
    runtime.WriteDword(output, 0xffffffffu);
    runtime.WriteDword(output + 8u, 0u);
    runtime.WriteDword(output + 12u, 0u);
    runtime.WriteDword(output + 16u, runtime.FposLocale());
    return output;
}

std::uint32_t CeguiStreamBufferOperations::SeekOffFailure(
    const std::uint32_t output, ICeguiStreamBufferRuntime& runtime) {
    return WriteFailurePosition(output, runtime);
}

std::uint32_t CeguiStreamBufferOperations::InitializeFailurePosition(
    const std::uint32_t output, ICeguiStreamBufferRuntime& runtime) {
    return WriteFailurePosition(output, runtime);
}

std::uint32_t CeguiStreamBufferOperations::SetBuffer(
    const std::uint32_t streambuf, std::uint32_t /*ignored_buffer*/,
    std::uint32_t /*ignored_size*/) noexcept {
    return streambuf;
}

std::int32_t CeguiStreamBufferOperations::PeekChar(
    const std::uint32_t streambuf, ICeguiStreamBufferRuntime& runtime) {
    const auto cursor_address = runtime.ReadDword(streambuf + 28u);
    const auto cursor = runtime.ReadDword(cursor_address);
    if (cursor != 0u) {
        const auto count_address = runtime.ReadDword(streambuf + 44u);
        const auto count = runtime.ReadDword(count_address);
        if (cursor < cursor + count) {
            return runtime.ReadByte(cursor);
        }
    }
    const auto value = runtime.Underflow(streambuf);
    return runtime.ApplyUnderflowResult(streambuf, value);
}

}  // namespace pal4
