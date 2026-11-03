#include "pal4/csb_buffer_reader.h"

namespace pal4 {

bool CsbBufferReader::Read(const char* source, std::uint32_t& size,
                           std::uint32_t& data,
                           ICsbBufferRuntime& runtime) {
    const auto context = runtime.CreateContext(2u, 1u, source);
    if (context == 0u) return false;

    runtime.ReadSize(context, size);
    data = runtime.Allocate(size);
    runtime.ReadBytes(context, data, size);
    runtime.CloseContext(context, 0u);
    return true;
}

}  // namespace pal4
