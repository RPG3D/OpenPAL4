#include "pal4/xml_buffer_release.h"

#include "pal4/compact_string_buffer.h"

namespace pal4 {

std::uint32_t XmlBufferRelease::Release(
    const std::uint32_t object, IXmlParserNodeRuntime& runtime) {
    const auto block = runtime.ReadWord(object);
    return block == CompactStringBuffer::kEmptyBlock ?
        block : runtime.Free(block);
}

}  // namespace pal4
