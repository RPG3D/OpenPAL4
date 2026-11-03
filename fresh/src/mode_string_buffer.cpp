#include "pal4/mode_string_buffer.h"

#include <cstring>
#include <stdexcept>

namespace pal4 {

std::uint32_t ModeStringBufferRuntimeStub::CStringLength(
    const char* source) {
    return static_cast<std::uint32_t>(std::strlen(source));
}
void ModeStringBufferRuntimeStub::ThrowLength() {
    throw std::length_error("mode string buffer length");
}
void ModeStringBufferRuntimeStub::SetErrorFlag(
    ModeStringBufferRecord&, std::uint8_t) {}
void ModeStringBufferRuntimeStub::Reallocate(
    ModeStringBufferRecord& record, const std::uint32_t length) {
    record.buffer.data = next_address_;
    record.buffer.capacity = length;
    next_address_ += length + 1u;
}
void ModeStringBufferRuntimeStub::Copy(
    const std::uint32_t destination, const char* source,
    const std::uint32_t length) {
    for (std::uint32_t index = 0u; index < length; ++index) {
        bytes_[destination + index] = static_cast<std::uint8_t>(source[index]);
    }
}
void ModeStringBufferRuntimeStub::WriteByte(
    const std::uint32_t address, const std::uint8_t value) {
    bytes_[address] = value;
}
std::uint8_t ModeStringBufferRuntimeStub::ReadByte(
    const std::uint32_t address) {
    return bytes_.at(address);
}

ModeStringBufferRecord& ModeStringBuffer::InitializeCString(
    ModeStringBufferRecord& record, const char* source,
    const std::uint8_t mode, IModeStringBufferRuntime& runtime) {
    record.mode_word = (record.mode_word & 0xffffff00u) | mode;
    record.buffer = {};
    const auto length = runtime.CStringLength(source);
    if (length > StringBuffer::MaxSize()) {
        runtime.ThrowLength();
    }
    runtime.SetErrorFlag(record, 1u);
    if (length == 0u) {
        return record;
    }
    runtime.Reallocate(record, length);
    runtime.Copy(record.buffer.data, source, length);
    record.buffer.length = length;
    runtime.WriteByte(record.buffer.data + length, 0u);
    return record;
}

}  // namespace pal4
