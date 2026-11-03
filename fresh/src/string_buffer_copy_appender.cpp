#include "pal4/string_buffer_copy_appender.h"

#include <limits>
#include <stdexcept>
#include <string>

namespace pal4 {

std::uint32_t StringBufferCopyAppender::CopyAndAppend(
    const std::uint32_t destination, const std::uint32_t source,
    const std::uint32_t tail, IStringBufferCopyAppendRuntime& runtime) {
    std::string text(runtime.CopySource(source));
    const auto count = runtime.TailLength(tail);
    if (text.size() >= std::numeric_limits<std::uint32_t>::max() - count)
        throw std::length_error("string buffer append exceeds 32-bit length");

    if (count != 0u && runtime.Allocate(
                           static_cast<std::uint32_t>(text.size()) + count)) {
        const auto bytes = runtime.TailBytes(tail);
        if (bytes.size() < count)
            throw std::invalid_argument("tail buffer shorter than declared length");
        text.append(bytes.data(), count);
    }
    runtime.WriteDestination(destination, text);
    runtime.ReleaseTemporary(text);
    return destination;
}

std::uint32_t StringBufferCopyAppender::CopyAndAppendCString(
    const std::uint32_t destination, const std::uint32_t source,
    const std::uint32_t c_string,
    IStringBufferCopyAppendRuntime& runtime) {
    return CopyAndAppend(destination, source, c_string, runtime);
}

}  // namespace pal4
