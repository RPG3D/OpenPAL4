#include "pal4/data_stream_buffer.h"

namespace pal4 {

void DataStreamBuffer::ReadInto(
    void* stream, std::uint8_t*& output,
    IDataStreamBufferRuntime& runtime) {
    if (!stream)
        return;

    if (output)
        runtime.Free(output);

    std::int32_t length = 0;
    runtime.Read(stream, &length, sizeof(length));
    if (length <= 0)
        return;

    output = runtime.Allocate(static_cast<std::uint32_t>(length));
    runtime.Read(stream, output, static_cast<std::uint32_t>(length));
    output[length - 1] = 0;
}

}  // namespace pal4
