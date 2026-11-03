#include "pal4/data_stream_string_reader.h"

#include <memory>
#include <new>
#include <stdexcept>
#include <string_view>

namespace pal4 {

void DataStreamStringReader::Read(const std::uint32_t context,
                                  const std::uint32_t output,
                                  IDataStreamStringRuntime& runtime) {
    if (context == 0u) return;
    std::uint32_t size = 0u;
    runtime.ReadData(context, &size, sizeof(size));
    if (size == 0u)
        throw std::length_error("native DataStream_ReadString writes before a zero-length buffer");
    char* buffer = runtime.Allocate(size);
    if (buffer == nullptr) throw std::bad_alloc{};
    const auto release = [&runtime](char* bytes) { runtime.Free(bytes); };
    std::unique_ptr<char, decltype(release)> owned(buffer, release);
    runtime.ReadData(context, buffer, size);
    buffer[size - 1u] = '\0';
    runtime.InitializeString(output, buffer,
                             std::string_view(buffer).size());
}

}  // namespace pal4
